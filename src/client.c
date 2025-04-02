#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "colors.h"
#include "help.h"
#include "utils.h"

static int server_socket;
static struct sockaddr_in server_address;
static char server_response[1024];
static char uprompt[1024];
static ssize_t bytes_received;

static void halt(int status) {
    if (status == EXIT_SUCCESS) {
        send(server_socket, uprompt, strlen(uprompt), 0);
        close(server_socket);
        exit(EXIT_SUCCESS);
    }
    else {
        close(server_socket);
        exit(EXIT_FAILURE);
    }    
}

void connect_to_server(int port, char addr[]) {
    // create socket, AF_INET - IPv4, SOCK_STREAM - TCP, 0 - IP
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // specify an address for the socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &server_address.sin_addr) <= 0) {
        perror(RED "Error: inet_pton()" RESET);
        halt(EXIT_FAILURE);
    }

    if (connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        perror(RED "Error: connect()" RESET);
        halt(EXIT_FAILURE);
    }
}

static void quit(void) {
    printf(CYAN "Session terminated.\n" RESET);
    close(server_socket);
    exit(EXIT_SUCCESS);
}

static void help(void) {
    printf(help_header);
    printf(help_client);
}

static void receive(void) {
    memset(server_response, 0, sizeof(server_response));  // Clear buffer
    bytes_received = recv(server_socket, &server_response, sizeof(server_response) - 1, 0);
    if (bytes_received <= 0) {
        // If recv() returns 0, the server disconnected
        if (bytes_received == 0) {
            printf(RED "Server disconnected.\n" RESET);
        } 
        // If recv() returns -1, an error occurred
        else {
            perror(RED "Error: recv()" RESET);
        }
        halt(EXIT_FAILURE);
    }
}

int client(int port, char addr[]) {    
    connect_to_server(port, addr);

    while (true) {
        receive();
        fprintf(stdout, "%s", server_response);

        memset(uprompt, 0, sizeof(uprompt));  // Clear buffer
        if (fscanf(stdin, " %1023s", uprompt) == EOF) {
            perror(RED "Error: fscanf()" RESET);
        }

        if (strcmp(uprompt, "help") == 0) {
            help();
        }
        else if (strcmp(uprompt, "halt") == 0) {
            halt(EXIT_SUCCESS);
        }
        else if (strcmp(uprompt, "quit") == 0) {
            quit();
        }
        else {
            send(server_socket, uprompt, strlen(uprompt), 0);
            receive();
            fprintf(stdout, "%s\n", server_response);
        }
    }
}