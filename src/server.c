#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include "utils.h"
#include "help.h"
#include "utils.h"
#include "colors.h"
#include "default.h"
#include "help.h"

typedef struct DPrompt {
    char* username;
    char hostname[256];
    char hostdate[11];
    char hosttime[9];
    char cwd[SIZE];
} DPrompt;

typedef struct Server {
    int server_socket;
    int client_socket;
    struct sockaddr_in socket_addr_in;
    struct sockaddr_un socket_addr_un;
    char server_response[SIZE*10];
} Server;

typedef struct Pipe {
    char* tokens;
} Pipe;

typedef struct Task {
    int pipe_count;
    Pipe* pipes;
    char* tokens;
} Task;

typedef struct UPrompt {
    ssize_t bytes_received;
    char request[SIZE];
    char response[SIZE*10];
    int task_count;
    Task* tasks;
} UPrompt;

Server s;
UPrompt clt;
UPrompt cli;
DPrompt d;

pthread_mutex_t mutex;

static void halt(int status) {
    close(s.server_socket);
    close(s.client_socket);
    exit(status);
}

static void help(void) {
    printf(help_header);
    printf(help_server);
}

static void cd(char* path) {
    chdir(path);
}

void bind_socket_in(char* port, char* addr) {
    if ((s.server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror(RED "Error: socket()" RESET);
        halt(EXIT_FAILURE);
    }

    s.socket_addr_in.sin_family = AF_INET;
    s.socket_addr_in.sin_port = htons(atoi(port));
    if (inet_pton(AF_INET, addr, &s.socket_addr_in.sin_addr) <= 0) {
        perror(RED "Error: inet_pton()" RESET);
        halt(EXIT_FAILURE);
    }

    if (bind(s.server_socket, (struct sockaddr*)&s.socket_addr_in, sizeof(s.socket_addr_in)) != 0) {
        perror(RED "Error: bind()" RESET);
        halt(EXIT_FAILURE);
    }
}

void bind_socket_un(char* socket_path) {
    if ((s.server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror(RED "Error: socket()" RESET);
        halt(EXIT_FAILURE);
    }

    s.socket_addr_un.sun_family = AF_UNIX;
    strncpy(s.socket_addr_un.sun_path, socket_path, sizeof(s.socket_addr_un.sun_path) - 1);

    unlink(socket_path);

    if (bind(s.server_socket, (struct sockaddr*)&s.socket_addr_un, sizeof(s.socket_addr_un)) != 0) {
        perror(RED "Error: bind()" RESET);
        halt(EXIT_FAILURE);
    }
}

void bind_socket(char* port, char* addr, char* socket_path) {
    if (socket_path == NULL) {
        bind_socket_in(port, addr);
    }
    else {
        bind_socket_un(socket_path);
    }

    // listen for connections on a socket
    if (listen(s.server_socket, 5) != 0) {
        perror(RED "Error: listen()" RESET);
        halt(EXIT_FAILURE);
    }

    if (socket_path == NULL) {
        printf(GREEN "Server is listening on %s:%s.\n" RESET, addr, port);
    }
    else {
        printf(GREEN "Server is listening on <%s>.\n" RESET, socket_path);
    }
}

void accept_connection(void) {
    // char* client_addr;
    // char* client_port;
    // accept a connection on a socket
    if ((s.client_socket = accept(s.server_socket, NULL, NULL)) == -1) {
        perror(RED "Error: accept()" RESET);
        halt(EXIT_FAILURE);
    }
    else {
        fprintf(stdout, CYAN "Client connected.\n" RESET);
    }
}

static int receive(UPrompt* u) {
    memset(u->request, 0, sizeof(u->request));  // Clear buffer
    u->bytes_received = recv(s.client_socket, u->request, sizeof(u->request) - 1, 0);
    if (u->bytes_received <= 0) {
        // If recv() returns 0, the server disconnected
        if (u->bytes_received == 0) {
            printf(RED "Client disconnected.\n" RESET);
            close(s.client_socket);
            return -1;
        } 
        // If recv() returns -1, an error occurred
        else {
            perror(RED "Error: recv()" RESET);
            halt(EXIT_FAILURE);
        }
    }
    return 0;
}

void send_dprompt(UPrompt* u) {
    default_prompt(&d);
    memset(u->response, 0, sizeof(u->response));  // Clear response buffer
    sprintf(u->response, YELLOW "%s|%s|%s@%s|%s" RESET "\n> ", d.hostdate, d.hosttime, d.username, d.hostname, d.cwd);
    send(s.client_socket, u->response, strlen(u->response), 0);
}

void print_dprompt(UPrompt* u) {
    default_prompt(&d);
    memset(u->response, 0, sizeof(u->response));  // Clear response buffer
    printf(YELLOW "%s|%s|%s@%s|%s" RESET "\n> ", d.hostdate, d.hosttime, d.username, d.hostname, d.cwd);
    fflush(stdout);
}

void clear_uprompt(UPrompt* u) {
    for (int i = 0; i < u->task_count - 1; i++) {
        if (u->tasks[i].pipes != NULL) {
            free(u->tasks[i].pipes);
            u->tasks[i].pipes = NULL;
        }
    }

    if (u->tasks != NULL) {
        free(u->tasks);
        u->tasks = NULL;
    }
}

void parse_semicolon(UPrompt* u) {
    char* token;
    int buf_pos = 0;
    char* request_ptr;
    u->task_count = 1;
    u->tasks = (Task*)calloc(u->task_count, sizeof(Task));
    if (!u->tasks) {
        perror(RED "Error: calloc(semicolon)" RESET);
        halt(EXIT_FAILURE);
    }

    token = strtok_r(u->request, ";", &request_ptr);
    while (token != NULL) {
        u->tasks[buf_pos].tokens = token;
        buf_pos++;

        if (buf_pos >= u->task_count) {
            u->task_count++;
            u->tasks = (Task*)realloc(u->tasks, u->task_count * sizeof(Task));
            if (!u->tasks) {
                perror(RED "Error: realloc(semicolon)" RESET);
                halt(EXIT_FAILURE);
            }
        }
        token = strtok_r(NULL, ";", &request_ptr);
    }
    u->tasks[buf_pos].tokens = NULL; 
}

void parse_pipeline(Task *task) {
    char* token;
    int buf_pos = 0;
    char* request_ptr;
    task->pipe_count = 1;
    task->pipes = (Pipe*)calloc(task->pipe_count, sizeof(Pipe));
    if (!task->pipes) {
        perror(RED "Error: calloc(pipeline)" RESET);
        halt(EXIT_FAILURE);
    }

    token = strtok_r(task->tokens, "|", &request_ptr);
    while (token != NULL) {
        task->pipes[buf_pos].tokens = token;
        buf_pos++;

        if (buf_pos >= task->pipe_count) {
            task->pipe_count++;
            task->pipes = (Pipe*)realloc(task->pipes, task->pipe_count * sizeof(Pipe));
            if (!task->pipes) {
                perror(RED "Error: realloc(pipeline)" RESET);
                halt(EXIT_FAILURE);
            }
        }
        token = strtok_r(NULL, "|", &request_ptr);
    }
    task->pipes[buf_pos].tokens = NULL; 
}

void parse_request(UPrompt* u) {
    parse_semicolon(u);
    for (int i = 0; i < u->task_count - 1; i++) {
        parse_pipeline(&u->tasks[i]);
    }
}

int internal_commands(char** tokens) {
    if (tokens[0] == NULL) {
        return 1;
    }
    else if (strcmp(tokens[0], "cd") == 0) {
        cd(tokens[1]);
        return 1;
    }
    else if (strcmp(tokens[0], "halt") == 0) {
        halt(EXIT_SUCCESS);
    }
    return 0;
}

void redirect_file(char* tokens, char* tmp_string, int* input_fd, int* output_fd, char* redirection, int* task_pipe_fd) {
    char* redirections[100] = {NULL};
    tmp_string = strdup(tokens);
    get_tokens(tmp_string, redirections, " ");

    if (dup2(task_pipe_fd[1], STDERR_FILENO) == -1) {
        perror(RED "Error: pipe(redirect_file)" RESET);
        exit(EXIT_FAILURE);
    }

    for (int j = 0; redirections[j] != NULL; j++) {
        if (strcmp(redirections[j], redirection) == 0 && redirections[j+1] != NULL) {
            if (strcmp(redirection, ">") == 0) {
                *output_fd = open(redirections[j+1], O_WRONLY | O_CREAT);
                if (*output_fd == -1) {
                    perror(RED "Error: open(O_WRONLY | O_CREAT)" RESET);
                    free(tmp_string); 
                    halt(EXIT_FAILURE);
                }
            }
            else if (strcmp(redirection, ">>") == 0) {
                *output_fd = open(redirections[j+1], O_APPEND | O_CREAT | O_WRONLY);
                if (*output_fd == -1) {
                    perror(RED "Error: open(O_APPEND | O_CREAT | O_WRONLY)" RESET);
                    free(tmp_string); 
                    halt(EXIT_FAILURE);
                }
            }
            else if (strcmp(redirection, "<") == 0) {
                *input_fd = open(redirections[j+1], O_RDONLY);
                if (*input_fd == -1) {
                    perror(RED "Error: open(O_RDONLY)" RESET);
                    free(tmp_string); 
                    halt(EXIT_FAILURE);
                }
            }
            if (*input_fd == -1) {
                if (dup2(*output_fd, STDOUT_FILENO) == -1) {
                    perror(RED "Error: dup2(*output_fd, STDOUT_FILENO)" RESET);
                    free(tmp_string); 
                    halt(EXIT_FAILURE);
                }
            }
            else {
                if (dup2(*input_fd, STDIN_FILENO) == -1) {
                    perror(RED "Error: dup2(*input_fd, STDIN_FILENO)" RESET);
                    free(tmp_string); 
                    halt(EXIT_FAILURE);
                }
            }
            break;
        }
    }       
    free(tmp_string); 
    tmp_string = NULL;   
}

void exec_pipe(UPrompt* u, Pipe* pipes, int pipe_count) {
    int task_pipe_fd[2];
    int input_fd = -1;
    int output_fd = -1;

    char* tmp_string = NULL;

    if (pipe(task_pipe_fd) == -1) {
        perror(RED "Error: pipe(exec_pipe)" RESET);
        halt(EXIT_FAILURE);
    }

    int pipefd[2 * (pipe_count - 1)];
    if (pipe_count > 1) {
        for (int i = 0; i < pipe_count - 1; i++) {
            if (pipe(pipefd + i * 2) == -1) {
                perror(RED "Error: pipe(exec_pipe)" RESET);
                halt(EXIT_FAILURE);
            }
        }
    }
    
    for (int i = 0; i < pipe_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror(RED "Error: fork(exec_pipe)" RESET);
            halt(EXIT_FAILURE);
        }

        if (pid == 0) {
            if (i == 0) {
                redirect_file(pipes[i].tokens, tmp_string, &input_fd, &output_fd, "<", task_pipe_fd);     
            }
            if (i > 0) {
                if (dup2(pipefd[(i - 1) * 2], STDIN_FILENO) == -1) {
                    perror(RED "Error: pipe(exec_pipe)" RESET);
                    exit(EXIT_FAILURE);
                }
            }
            if (i < pipe_count - 1) { 
                if (output_fd == -1 && dup2(pipefd[i * 2 + 1], STDOUT_FILENO) == -1) {
                    perror("dup2 (stdout)");
                    exit(EXIT_FAILURE);
                }
            }
            if (i == pipe_count - 1 || pipe_count == 1) {
                redirect_file(pipes[i].tokens, tmp_string, &input_fd, &output_fd, ">", task_pipe_fd);  
                redirect_file(pipes[i].tokens, tmp_string, &input_fd, &output_fd, ">>", task_pipe_fd); 

                close(task_pipe_fd[0]);
                if (output_fd == -1 && (dup2(task_pipe_fd[1], STDOUT_FILENO) == -1 || dup2(task_pipe_fd[1], STDERR_FILENO) == -1)) {
                    perror("dup2 (stdout)");
                    halt(EXIT_FAILURE);
                }
                close(task_pipe_fd[1]);
            }

            for (int j = 0; j < 2 * (pipe_count - 1); j++) {
                close(pipefd[j]);
            }

            char* tmp_string = strtok(pipes[i].tokens, "<>");
            char* tokens[100] = {NULL};
            get_tokens(tmp_string, tokens, DELIMS);
            // free(tmp_string);

            if (internal_commands(tokens)) {
                return;
            }

            if (execvp(tokens[0], tokens) == -1) {
                perror(RED "Error: execvp(exec_pipe)" RESET);
                printf("\r");
                halt(EXIT_FAILURE);
            }
        }
        close(input_fd);
        close(output_fd);
    }

    for (int i = 0; i < 2 * (pipe_count - 1); i++) {
        close(pipefd[i]);
    }

    close(task_pipe_fd[1]);
    if (output_fd == -1) {
        int nbytes;
        int total_bytes_read = strlen(u->response);
        while ((nbytes = read(task_pipe_fd[0], u->response + total_bytes_read, SIZE - total_bytes_read)) > 0) {
            total_bytes_read += nbytes;
        }
        if (nbytes == -1) {
            perror(RED "Error: read()" RESET);
            halt(EXIT_FAILURE);
        }
    }
    close(task_pipe_fd[0]);

    for (int i = 0; i < pipe_count; i++) {
        wait(NULL);
    }
    if (tmp_string != NULL) {
        free(tmp_string);
    }
    close(input_fd);
    close(output_fd);
}

void exec_task(UPrompt* u, Task* task) {
    exec_pipe(u, task->pipes, task->pipe_count - 1);
}

void process_request(UPrompt* u) {
    memset(u->response, 0, sizeof(u->response));  // Clear response buffer

    for (int i = 0; i < u->task_count && u->tasks[i].tokens != NULL; i++) {
        exec_task(u, &u->tasks[i]);
    }
    clear_uprompt(u);
}

void recv_eval_req_loop(void) {
    while (true) {
        send_dprompt(&clt);

        if (receive(&clt) != 0) {
            break;
        }
        else {
            // printf("%s\n", clt.request);
            parse_request(&clt);
            process_request(&clt);
            if (strlen(clt.response) == 0) {
                strcpy(clt.response, "\r");
            }
            send(s.client_socket, clt.response, strlen(clt.response), 0);
        }
    }
}

void* cli_input(void* arg) {
    (void)arg;
    while (true) {
        print_dprompt(&cli);

        ssize_t bytes_read;
        size_t uprompt_len;
        char* request = NULL;

        fflush(stdin);
        if ((bytes_read = getline(&request, &uprompt_len, stdin)) == -1) {
            perror(RED "Error: getline()" RESET);
            halt(EXIT_FAILURE);
        }
        request[bytes_read-1] = '\0'; // remove '\n'

        strncpy(cli.request, request, SIZE - 1);
        
        if (strcmp(cli.request, "help") == 0) {
            help();
            memset(cli.request, 0, strlen(cli.request));  // Clear buffer
            print_dprompt(&cli);
        }
        else if (strcmp(cli.request, "halt") == 0) {
            halt(EXIT_SUCCESS);
        }
        else {
            parse_request(&cli);
            process_request(&cli);
            if (strlen(cli.response) != 0) {
                printf("%s", cli.response);
            }
        }
    }
    return NULL;
}

void server(char* port, char* addr, char* socket_path) { 
    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, cli_input, NULL) != 0) {
        perror(RED "Error: pthread_create()" RESET);
        halt(EXIT_FAILURE);
    }

    bind_socket(port, addr, socket_path);

    while (true) {
        accept_connection();

        recv_eval_req_loop();
    }

    pthread_join(input_thread, NULL);

    halt(EXIT_SUCCESS);
}