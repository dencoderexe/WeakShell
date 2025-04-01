#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include "help.h"
#include "utils.h"
#include "colors.h"

static int server_socket;
static int client_socket;
static struct sockaddr_in server_address;
static char client_request[1024] = {"\0"};
static char server_response[1024] = "Message received";

// static int halt(void) {
//     close(server_socket);
//     close(client_socket);
//     return EXIT_SUCCESS;
// }

int connection_wait(void) {
    // create socket, AF_INET - IPv4, SOCK_STREAM - TCP, 0 - IP
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror(RED "Error: socket()" RESET);
        return -1;
    }

    // specify an address for the socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket
    if(bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        perror(RED "Error: bind()" RESET);
        return -1;
    }

    // listen for connections on a socket
    if(listen(server_socket, 5) != 0) {
        perror(RED "Error: listen()" RESET);
        return -1;
    }

    // accept a connection on a socket
    if((client_socket = accept(server_socket, NULL, NULL)) == -1) {
        perror(RED "Error: accept()" RESET);
        return -1;
    }

    return 0;
}

int server(void) {
    if(connection_wait() != 0) {
        close(server_socket);
        return EXIT_FAILURE;
    }
    else {
        fprintf(stdout, GREEN "Connection established.\n" RESET);
    }
    
    while (true)
    {
        memset(client_request, 0, sizeof(client_request));  // Clear buffer
        ssize_t bytes_received = recv(client_socket, client_request, sizeof(client_request) - 1, 0);
        if (bytes_received <= 0) {
            // If recv() returns 0, the client disconnected
            if (bytes_received == 0) {
                printf(RED "Client disconnected.\n" RESET);
            } 
            // If recv() returns -1, an error occurred
            else {
                perror(RED "Error: recv()" RESET);
            }
            close(client_socket);
            close(server_socket);
            return EXIT_FAILURE;
        }

        printf("%s\n", client_request);
        send(client_socket, server_response, strlen(server_response), 0);
    }
    
    return EXIT_SUCCESS;
}