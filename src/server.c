#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "utils.h"
#include "help.h"
#include "utils.h"
#include "colors.h"

static int server_socket;
static struct sockaddr_in server_address;
static char server_response[1024] = "Message received";
static int client_socket;
static char client_request[1024] = {0};
static char dprompt[1312];
static ssize_t bytes_received;

static void halt(int status) {
    close(server_socket);
    close(client_socket);
    exit(status);
}

void bind_socket(int port, char addr[]) {
    // create socket, AF_INET - IPv4, SOCK_STREAM - TCP, 0 - IP
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror(RED "Error: socket()" RESET);
        halt(EXIT_FAILURE);
    }

    // specify address and port for the socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &server_address.sin_addr) <= 0) {
        perror(RED "Error: inet_pton()" RESET);
        halt(EXIT_FAILURE);
    }

    // bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        perror(RED "Error: bind()" RESET);
        halt(EXIT_FAILURE);
    }

    // listen for connections on a socket
    if (listen(server_socket, 5) != 0) {
        perror(RED "Error: listen()" RESET);
        halt(EXIT_FAILURE);
    }
}

void connection_accept(void) {
    // accept a connection on a socket
    if ((client_socket = accept(server_socket, NULL, NULL)) == -1) {
        perror(RED "Error: accept()" RESET);
        halt(EXIT_FAILURE);
    }
    else {
        fprintf(stdout, CYAN "Client connected.\n" RESET);
    }
}

static int receive(void) {
    memset(client_request, 0, sizeof(client_request));  // Clear buffer
    bytes_received = recv(client_socket, client_request, sizeof(client_request) - 1, 0);
    if (bytes_received <= 0) {
        // If recv() returns 0, the server disconnected
        if (bytes_received == 0) {
            printf(RED "Client disconnected.\n" RESET);
            close(client_socket);
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

void server(int port, char addr[]) {   
    bind_socket(port, addr);

    while (true) {
        connection_accept();

        while (true)
        {
            default_prompt(dprompt);
            send(client_socket, dprompt, strlen(dprompt), 0);

            if (receive() != 0) {
                break;
            }
            else if (strcmp(client_request, "halt") == 0) {
                halt(EXIT_SUCCESS);
            }

            printf("%s\n", client_request);
            send(client_socket, server_response, strlen(server_response), 0);
        }
    }

    halt(EXIT_SUCCESS);
}