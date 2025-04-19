#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "default.h"
#include "colors.h"
#include "help.h"
#include "utils.h"

typedef struct Client
{
    int server_socket;
    struct sockaddr_in socket_addr_in;
    struct sockaddr_un socket_addr_un;
    char server_response[SIZE*10];
    char* uprompt;
    size_t uprompt_len;
    ssize_t bytes_read;
    ssize_t bytes_received;
} Client;

Client c;

static void halt(int status) {
    if (status == EXIT_SUCCESS) {
        send(c.server_socket, c.uprompt, strlen(c.uprompt), 0);
        close(c.server_socket);
        free(c.uprompt);
        exit(EXIT_SUCCESS);
    }
    else {
        close(c.server_socket);
        free(c.uprompt);
        exit(EXIT_FAILURE);
    }    
}

void connect_to_server(char* port, char* addr, char* socket_path) {
    if (socket_path == NULL) {
        c.server_socket = socket(AF_INET, SOCK_STREAM, 0);

        c.socket_addr_in.sin_family = AF_INET;
        c.socket_addr_in.sin_port = htons(atoi(port));
        if (inet_pton(AF_INET, addr, &c.socket_addr_in.sin_addr) <= 0) {
            perror(RED "Error: inet_pton()" RESET);
            halt(EXIT_FAILURE);
        }

        if (connect(c.server_socket, (struct sockaddr*)&c.socket_addr_in, sizeof(c.socket_addr_in)) != 0) {
            perror(RED "Error: connect()" RESET);
            halt(EXIT_FAILURE);
        }
    }
    else {
        c.server_socket = socket(AF_UNIX, SOCK_STREAM, 0);

        c.socket_addr_un.sun_family = AF_UNIX;
        strncpy(c.socket_addr_un.sun_path, socket_path, sizeof(c.socket_addr_un.sun_path) - 1);

        if (connect(c.server_socket, (struct sockaddr*)&c.socket_addr_un, sizeof(c.socket_addr_un)) != 0) {
            perror(RED "Error: connect()" RESET);
            halt(EXIT_FAILURE);
        }
    }
}

static void quit(void) {
    printf(CYAN "Session terminated.\n" RESET);
    close(c.server_socket);
    free(c.uprompt);
    exit(EXIT_SUCCESS);
}

static void help(void) {
    printf(help_header);
    printf(help_client);
}

static void receive(void) {
    memset(c.server_response, 0, sizeof(c.server_response));  // Clear buffer
    c.bytes_received = recv(c.server_socket, &c.server_response, sizeof(c.server_response) - 1, 0);
    if (c.bytes_received <= 0) {
        // If recv() returns 0, the server disconnected
        if (c.bytes_received == 0) {
            printf(RED "Server disconnected.\n" RESET);
        } 
        // If recv() returns -1, an error occurred
        else {
            perror(RED "Error: recv()" RESET);
        }
        halt(EXIT_FAILURE);
    }
}

int client(char* port, char* addr, char* socket_path) {    
    connect_to_server(port, addr, socket_path);

    while (true) {
        receive();
        fprintf(stdout, "%s", c.server_response);

        if ((c.bytes_read = getline(&c.uprompt, &c.uprompt_len, stdin)) == -1) {
            perror(RED "Error: getline()" RESET);
            halt(EXIT_FAILURE);
        }

        // remove '\n'
        c.uprompt[c.bytes_read-1] = '\0';

        if (strcmp(c.uprompt, "help") == 0) {
            help();
        }
        else if (strcmp(c.uprompt, "halt") == 0) {
            halt(EXIT_SUCCESS);
        }
        else if (strcmp(c.uprompt, "quit") == 0) {
            quit();
        }
        else {
            send(c.server_socket, c.uprompt, c.bytes_read, 0);
            receive();
            fprintf(stdout, "%s", c.server_response);
        }
    }

    halt(EXIT_SUCCESS);
}