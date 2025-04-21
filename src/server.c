#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include "utils.h"
#include "help.h"
#include "utils.h"
#include "colors.h"
#include "default.h"
#include "help.h"

// Structure for representing the default prompt (username, hostname, etc.)
typedef struct DPrompt {
    char* username;    // Username
    char hostname[256]; // Hostname
    char hostdate[11];  // Host date
    char hosttime[9];   // Host time
    char cwd[SIZE];     // Current working directory
} DPrompt;

// Structure for the server configuration (sockets, client connections, etc.)
typedef struct Server {
    int server_socket;           // Server socket descriptor
    int client_socket[MAX_CLIENTS]; // Array of client sockets
    fd_set client_fds;           // Set of client file descriptors for select()
    int max_fd;                  // Maximum file descriptor value
    struct sockaddr_in socket_addr_in;  // IPv4 socket address
    struct sockaddr_un socket_addr_un;  // UNIX socket address
    char server_response[SIZE*10]; // Buffer for server responses
} Server;

// Structure for handling pipe tokens (used in command execution)
typedef struct Pipe {
    char* tokens;  // Tokens in the pipe
} Pipe;

// Structure representing a task with its pipes and tokens (commands)
typedef struct Task {
    int pipe_count;  // Number of pipes in the task
    Pipe* pipes;     // Array of pipes
    char* tokens;    // Tokens of the task
} Task;

// Structure for managing client request and response, including tasks
typedef struct UPrompt {
    ssize_t bytes_received;  // Number of bytes received from client
    char request[SIZE];      // Client's request
    char response[SIZE*10];  // Server's response
    int task_count;          // Number of tasks in the request
    Task* tasks;             // Array of tasks in the request
} UPrompt;

Server s;
UPrompt clt;
UPrompt cli;
DPrompt d;

// Function to send the default prompt to the client
void send_dprompt(UPrompt* u, int client_socket) {
    default_prompt(&d);
    memset(u->response, 0, sizeof(u->response));  // Clear response buffer
    sprintf(u->response, YELLOW "%s|%s|%s@%s|%s" RESET "\n> ", d.hostdate, d.hosttime, d.username, d.hostname, d.cwd);
    send(client_socket, u->response, strlen(u->response), 0);
}

// Function to print the default prompt to the console
void print_dprompt(UPrompt* u) {
    default_prompt(&d);
    memset(u->response, 0, sizeof(u->response));  // Clear response buffer
    printf(YELLOW "%s|%s|%s@%s|%s" RESET "\n> ", d.hostdate, d.hosttime, d.username, d.hostname, d.cwd);
    fflush(stdout);
}


// Function to abort connection for a client socket
void abort_connection(int client_socket) {
    struct sockaddr_in socket_addr;
    socklen_t addrlen = sizeof(socket_addr);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (s.client_socket[i] == client_socket) {
            if (getpeername(client_socket, (struct sockaddr *)&socket_addr, &addrlen) == 0) {
                printf(RED "Client %d from %s:%d was disconnected.\n" RESET, client_socket, inet_ntoa(socket_addr.sin_addr), ntohs(socket_addr.sin_port));
            }
            close(s.client_socket[i]);
            FD_CLR(s.server_socket, &s.client_fds);
            s.client_socket[i] = 0;
            break;
        }
    }
    s.max_fd = s.server_socket;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (s.client_socket[i] > s.max_fd) {
            s.max_fd = s.client_socket[i];
        }
    }
}

// Function to halt server, closing sockets and aborting client connections
static void halt(int status) {
    close(s.server_socket);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        abort_connection(s.client_socket[i]);
    }
    exit(status);
}

// Function to display active sockets and connections
void stat(void) {
    struct sockaddr_in socket_addr;
    socklen_t addrlen = sizeof(socket_addr);
    printf(YELLOW "\tActive sockets\n" RESET);
    printf("SOCKET\t\tADDR\t\t\tPORT\n");
    if (getsockname(s.server_socket, (struct sockaddr *)&socket_addr, &addrlen) == 0) {
        printf("[1]\t\t[%s]\t[%d]\n", inet_ntoa(socket_addr.sin_addr), ntohs(socket_addr.sin_port));
    }
    
    printf(YELLOW "\n\tActive connections\n" RESET);
    printf("CONNECTION\tADDR\t\t\tPORT\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {        
        if (s.client_socket[i] != 0 && getpeername(s.client_socket[i], (struct sockaddr *)&socket_addr, &addrlen) == 0) {
            printf("[%d]\t\t[%s]\t[%d]\n", s.client_socket[i], inet_ntoa(socket_addr.sin_addr), ntohs(socket_addr.sin_port));
        }
    }
}

// Function to display help information
static void help(void) {
    printf(help_header);
    printf(help_server);
}

// Function to change the current working directory
static void cd(char* path) {
    chdir(path);
}

// Function to bind a socket using IPv4 address and port
void bind_socket_in(char* port, char* addr) {
    if ((s.server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror(RED "Error: socket()" RESET);
        halt(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(s.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror(RED "Error: setsockopt()" RESET);
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

// Function to bind a socket using UNIX domain socket
void bind_socket_un(char* socket_path) {
    if ((s.server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror(RED "Error: socket()" RESET);
        halt(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(s.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror(RED "Error: setsockopt()" RESET);
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

// Function to bind the socket depending on the type (IPv4 or UNIX)
void bind_socket(char* port, char* addr, char* socket_path) {
    if (socket_path == NULL) {
        bind_socket_in(port, addr);
    }
    else {
        bind_socket_un(socket_path);
    }

    // listen for connections on a socket
    if (listen(s.server_socket, MAX_PENDINGS) != 0) {
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

// Function to accept an incoming client connection
void accept_connection(void) {
    int client_socket, addrlen;
    struct sockaddr_in socket_address;
    if ((client_socket = accept(s.server_socket, (struct sockaddr *)&socket_address, (socklen_t *)&addrlen)) < 0) {
        perror(RED "Error: accept()" RESET);
        halt(EXIT_FAILURE);
    }
    else {
        printf(CYAN "Client %d connected from %s:%d.\n" RESET, client_socket, inet_ntoa(socket_address.sin_addr), ntohs(socket_address.sin_port));
        print_dprompt(&cli);
        send_dprompt(&clt, client_socket);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        // If position is empty
        if (s.client_socket[i] == 0) {
            s.client_socket[i] = client_socket;
            break;
        }
    }
}

// Function to receive data from the client
static void receive(UPrompt* u, int client_socket) {
    memset(u->request, 0, sizeof(u->request));  // Clear buffer
    u->bytes_received = recv(client_socket, u->request, sizeof(u->request) - 1, 0);
    if (u->bytes_received <= 0) {
        // If recv() returns 0, the server disconnected
        if (u->bytes_received == 0) {
            
        } 
        // If recv() returns -1, an error occurred
        else {
            perror(RED "Error: recv()" RESET);
        }

        abort_connection(client_socket);
    }
}

// Function to clear the UPrompt structure
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

// Function to parse a client request by separating semicolon-delimited tasks
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

// Function to parse the pipeline commands (|) in a task
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

// Function to handle internal server commands like stat. Returns 1 if its special command and cannot be executed in child process.
// Returns 0 and execute command if its internal command.
int internal_command(char** tokens) {
    if (tokens[0] == NULL) {
        return 1;
    }
    else if (strcmp(tokens[0], "stat") == 0) {
        stat();
        return 0;
    }
    else if (strcmp(tokens[0], "abort") == 0) {
        return 1;
    }
    else if (strcmp(tokens[0], "cd") == 0) {
        return 1;
    }
    else if (strcmp(tokens[0], "halt") == 0) {
        return 1;
    }
    return 0;
}

void exec_spec_commands(UPrompt* u) {
    char* tmp_string = strdup(u->request);
    char tmp_delims[100] = "<>;|";
    strcat(tmp_delims, DELIMS);
    char* tokens[1000] = {NULL};
    get_tokens(tmp_string, tokens, tmp_delims);
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "abort") == 0 && tokens[1] != NULL) {      
            for (int i = 0; tokens[1][i] != '\0'; i++) {
                if (tokens[1][i] < 48 || tokens[1][i] > 57) {
                    printf("Wrong connection format.\n");
                    return;
                }
            }
             
            int client_socket = atoi(tokens[1]);
            abort_connection(client_socket);
            i++;
        }
        else if (strcmp(tokens[0], "cd") == 0 && tokens[1] != NULL) {
            cd(tokens[1]);
            i++;
        }
        else if (strcmp(tokens[0], "halt") == 0) {
            free(tmp_string);
            halt(EXIT_SUCCESS);
        }
    }
    free(tmp_string);
}

// Function to parse a client request into tasks and pipes
void parse_request(UPrompt* u) {
    exec_spec_commands(u);
    parse_semicolon(u);
    for (int i = 0; i < u->task_count - 1; i++) {
        parse_pipeline(&u->tasks[i]);
    }
}

// Function to handle file redirection (input/output)
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
                *output_fd = open(redirections[j+1], O_WRONLY | O_CREAT, 0666);
                if (*output_fd == -1) {
                    perror(RED "Error: open(O_WRONLY | O_CREAT)" RESET);
                    free(tmp_string); 
                    halt(EXIT_FAILURE);
                }
            }
            else if (strcmp(redirection, ">>") == 0) {
                *output_fd = open(redirections[j+1], O_APPEND | O_CREAT | O_WRONLY, 0666);
                if (*output_fd == -1) {
                    perror(RED "Error: open(O_APPEND | O_CREAT | O_WRONLY)" RESET);
                    free(tmp_string); 
                    halt(EXIT_FAILURE);
                }
            }
            else if (strcmp(redirection, "<") == 0) {
                *input_fd = open(redirections[j+1], O_RDONLY, 0666);
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

// Function to execute pipeline tasks with redirection and pipe handling
void exec_pipe(UPrompt* u, Pipe* pipes, int pipe_count) {
    int task_pipe_fd[2];
    int input_fd = -1;
    int output_fd = -1;

    pid_t pid;
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
        pid = fork();

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

            if (internal_command(tokens)) {
                close(input_fd);
                close(output_fd);
                exit(EXIT_SUCCESS);
            }

            if (execvp(tokens[0], tokens) == -1) {
                perror(RED "Error: execvp(exec_pipe)" RESET);
                printf("\r");
                halt(EXIT_FAILURE);
            }
            close(input_fd);
            close(output_fd);
        }
    }

    if (pid > 0) {
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
}

// Executes the task passed via pipes
void exec_task(UPrompt* u, Task* task) {
    exec_pipe(u, task->pipes, task->pipe_count - 1);
}

// Processes the user's request
void process_request(UPrompt* u) {
    memset(u->response, 0, sizeof(u->response));  // Clear response buffer

    for (int i = 0; i < u->task_count && u->tasks[i].tokens != NULL; i++) {
        exec_task(u, &u->tasks[i]);
    }
    clear_uprompt(u);
}

// Main loop for receiving and processing client requests
void recv_eval_req_loop(int client_socket) {
    memset(&cli, 0, sizeof(cli));

    receive(&clt, client_socket);

    parse_request(&clt);
    process_request(&clt);
    if (strlen(clt.response) == 0) {
        strcpy(clt.response, "\r");
    }
    send(client_socket, clt.response, strlen(clt.response), 0);
    send_dprompt(&clt, client_socket);
}

// Function for handling user input from the terminal
void* cli_input(void* arg) {
    (void)arg;
    while (true) {
        print_dprompt(&cli);

        ssize_t bytes_read;
        size_t uprompt_len;
        char* request = NULL;

        if ((bytes_read = getline(&request, &uprompt_len, stdin)) == -1) {
            perror(RED "Error: getline()" RESET);
            halt(EXIT_FAILURE);
        }
        request[bytes_read-1] = '\0'; // remove '\n'

        strncpy(cli.request, request, SIZE - 1);
        
        if (strcmp(cli.request, "help") == 0) {
            help();
            memset(cli.request, 0, strlen(cli.request));  // Clear buffer
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

// Main function for running the server
void server(char* port, char* addr, char* socket_path) { 
    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, cli_input, NULL) != 0) {
        perror(RED "Error: pthread_create()" RESET);
        halt(EXIT_FAILURE);
    }

    bind_socket(port, addr, socket_path);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        s.client_socket[i] = 0;
    }

    while (true) {
        FD_ZERO(&s.client_fds);
        FD_SET(s.server_socket, &s.client_fds);
        s.max_fd = s.server_socket;

        // Add child sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = s.client_socket[i];

            // If valid socket descriptor then add to read list
            if (fd > 0) {
                FD_SET(fd, &s.client_fds);
            }

            // Highest file descriptor number, need it for select()
            if (fd > s.max_fd) {
                s.max_fd = fd;
            }
        }

        int activity = select(s.max_fd + 1, &s.client_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror(RED "Error: select()" RESET);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(s.server_socket, &s.client_fds)) {
            accept_connection();
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = s.client_socket[i];
    
            if (FD_ISSET(fd, &s.client_fds)) {
                recv_eval_req_loop(fd);
            }
        }
    }

    pthread_join(input_thread, NULL);

    halt(EXIT_SUCCESS);
}