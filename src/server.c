#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "utils.h"
#include "help.h"
#include "utils.h"
#include "colors.h"
#include "default.h"

typedef struct DPrompt {
    char* username;
    char hostname[256];
    char hostdate[11];
    char hosttime[9];
    char cwd[SIZE];
} DPrompt;

typedef struct Server
{
    int server_socket;
    int client_socket;
    struct sockaddr_in server_address;
    char server_response[SIZE*2];
} Server;

typedef struct Request
{
    ssize_t bytes_received;
    char client_request[SIZE];
    int token_buffer_size;
    char** tokens;
} Request;

Server s;
Request r;
DPrompt d;

static void halt(int status) {
    close(s.server_socket);
    close(s.client_socket);
    exit(status);
}

void bind_socket(int port, char addr[]) {
    // create socket, AF_INET - IPv4, SOCK_STREAM - TCP, 0 - IP
    if ((s.server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror(RED "Error: socket()" RESET);
        halt(EXIT_FAILURE);
    }

    // specify address and port for the socket
    s.server_address.sin_family = AF_INET;
    s.server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &s.server_address.sin_addr) <= 0) {
        perror(RED "Error: inet_pton()" RESET);
        halt(EXIT_FAILURE);
    }

    // bind the socket
    if (bind(s.server_socket, (struct sockaddr*)&s.server_address, sizeof(s.server_address)) != 0) {
        perror(RED "Error: bind()" RESET);
        halt(EXIT_FAILURE);
    }

    // listen for connections on a socket
    if (listen(s.server_socket, 5) != 0) {
        perror(RED "Error: listen()" RESET);
        halt(EXIT_FAILURE);
    }
}

void accept_connection(void) {
    // accept a connection on a socket
    if ((s.client_socket = accept(s.server_socket, NULL, NULL)) == -1) {
        perror(RED "Error: accept()" RESET);
        halt(EXIT_FAILURE);
    }
    else {
        fprintf(stdout, CYAN "Client connected.\n" RESET);
    }
}

static int receive(void) {
    memset(r.client_request, 0, sizeof(r.client_request));  // Clear buffer
    r.bytes_received = recv(s.client_socket, r.client_request, sizeof(r.client_request) - 1, 0);
    if (r.bytes_received <= 0) {
        // If recv() returns 0, the server disconnected
        if (r.bytes_received == 0) {
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

void send_dprompt(void) {
    default_prompt(&d);
    memset(s.server_response, 0, sizeof(s.server_response));  // Clear response buffer
    sprintf(s.server_response, YELLOW "%s|%s|%s@%s|%s" RESET "\n> ", d.hostdate, d.hosttime, d.username, d.hostname, d.cwd);
    send(s.client_socket, s.server_response, strlen(s.server_response), 0);
}

void parse_request(void) {
    char* token;
    int buf_pos = 0;
    char* request_ptr;
    r.token_buffer_size = 1;
    r.tokens = (char**)malloc(r.token_buffer_size * sizeof(char*));
    if (!r.tokens) {
        perror(RED "Error: malloc()" RESET);
        halt(EXIT_FAILURE);
    }

    token = strtok_r(r.client_request, DELIMS, &request_ptr);
    while (token != NULL) {
        r.tokens[buf_pos] = token;
        buf_pos++;

        if (buf_pos >= r.token_buffer_size) {
            r.token_buffer_size++;
            r.tokens = (char**)realloc(r.tokens, r.token_buffer_size * sizeof(char*));
            if (!r.tokens) {
                perror(RED "Error: malloc()" RESET);
                halt(EXIT_FAILURE);
            }
        }
        token = strtok_r(NULL, DELIMS, &request_ptr);
    }
    r.tokens[buf_pos] = NULL; 
}

void process_request(void) {
    memset(s.server_response, 0, sizeof(s.server_response));  // Clear response buffer

    pid_t pid;
    int status;
    // pipefd[0] - read end of the pipe
    // pipefd[1] - write end of the pipe
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror(RED "Error: pipe()" RESET);
        halt(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror(RED "Error: dup2()" RESET);
            halt(EXIT_FAILURE);
        }
        close(pipefd[1]);

        if (execvp(r.tokens[0], r.tokens) == -1) {
            perror(RED "Error: execvp()" RESET);
            printf("\r");
            halt(EXIT_FAILURE);
        }
    }
    else if (pid < 0) {
        perror(RED "Error: fork()" RESET);
        halt(EXIT_FAILURE);
    }
    else {
        close(pipefd[1]);

        int nbytes;
        int total_bytes_read = 0;
        while ((nbytes = read(pipefd[0], s.server_response + total_bytes_read, SIZE - total_bytes_read)) > 0) {
            total_bytes_read += nbytes;
        }

        if (nbytes == -1) {
            perror(RED "Error: read()" RESET);
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);

        waitpid(pid, &status, 0);
    }
}

void recv_eval_req_loop(void) {
    while (true) {
        send_dprompt();

        if (receive() != 0) {
            break;
        }
        else if (strcmp(r.client_request, "halt") == 0) {
            halt(EXIT_SUCCESS);
        }
        else {
            printf("%s\n", r.client_request);
            parse_request();
            process_request();
            send(s.client_socket, s.server_response, strlen(s.server_response), 0);
        }            
    }
}

void server(int port, char addr[]) {   
    bind_socket(port, addr);

    while (true) {
        accept_connection();

        recv_eval_req_loop();
    }

    halt(EXIT_SUCCESS);
}