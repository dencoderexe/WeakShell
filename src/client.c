// Include necessary libraries for various functions and settings
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

// Structure for storing client-related data (socket, addresses, etc.)
typedef struct Client
{
    int server_socket;               // Socket for connection to the server
    struct sockaddr_in socket_addr_in; // IPv4 address structure for server
    struct sockaddr_un socket_addr_un; // Unix socket address structure
    char server_response[SIZE*10];     // Buffer to store server's response
    char* uprompt;                   // User input prompt buffer
    size_t uprompt_len;              // Length of the user prompt buffer
    ssize_t bytes_read;              // Number of bytes read from user input
    ssize_t bytes_received;          // Number of bytes received from the server
} Client;

Client c;

// Function to terminate client and server, sending current prompt (halt) to the server
// and closing the socket
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

// Function to connect the client to the server, either via IP or Unix socket
void connect_to_server(char* port, char* addr, char* socket_path) {
    if (socket_path == NULL) { // Connect via IP (IPv4)
        c.server_socket = socket(AF_INET, SOCK_STREAM, 0);

        c.socket_addr_in.sin_family = AF_INET;
        c.socket_addr_in.sin_port = htons(atoi(port)); // Convert port to network byte order
        if (inet_pton(AF_INET, addr, &c.socket_addr_in.sin_addr) <= 0) {
            perror(RED "Error: inet_pton()" RESET);
            halt(EXIT_FAILURE);
        }

        if (connect(c.server_socket, (struct sockaddr*)&c.socket_addr_in, sizeof(c.socket_addr_in)) != 0) {
            perror(RED "Error: connect()" RESET);
            halt(EXIT_FAILURE);
        }
    }
    else { // Connect via Unix socket
        c.server_socket = socket(AF_UNIX, SOCK_STREAM, 0);

        c.socket_addr_un.sun_family = AF_UNIX;
        strncpy(c.socket_addr_un.sun_path, socket_path, sizeof(c.socket_addr_un.sun_path) - 1);

        if (connect(c.server_socket, (struct sockaddr*)&c.socket_addr_un, sizeof(c.socket_addr_un)) != 0) {
            perror(RED "Error: connect()" RESET);
            halt(EXIT_FAILURE);
        }
    }
}

// Function to terminate the client session and close the socket
static void quit(void) {
    printf(CYAN "Session terminated.\n" RESET);
    close(c.server_socket);
    free(c.uprompt);
    exit(EXIT_SUCCESS);
}

// Function to display help information for the client
static void help(void) {
    printf(help_header);
    printf(help_client);
}

// Function to receive server response and handle errors
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

// Main client function to connect to the server and handle user commands
int client(char* port, char* addr, char* socket_path) {    
    connect_to_server(port, addr, socket_path);

    while (true) {
        receive(); // Receive the server's response
        fprintf(stdout, "%s", c.server_response); // Print server's response

        // Read user input (command)
        if ((c.bytes_read = getline(&c.uprompt, &c.uprompt_len, stdin)) == -1) {
            perror(RED "Error: getline()" RESET);
            halt(EXIT_FAILURE);
        }

        // Remove newline character from user input
        c.uprompt[c.bytes_read-1] = '\0';

        // Handle specific commands from the user
        if (strcmp(c.uprompt, "help") == 0) {
            help(); // Show help message
            memset(c.uprompt, 0, strlen(c.uprompt));  // Clear buffer
            send(c.server_socket, c.uprompt, c.bytes_read, 0); // Send to server
            receive(); // Receive response
            fprintf(stdout, "%s", c.server_response); // Print server's response
        }
        else if (strcmp(c.uprompt, "halt") == 0) {
            halt(EXIT_SUCCESS); // Shutdown client and server and close socket
        }
        else if (strcmp(c.uprompt, "quit") == 0) {
            quit(); // Terminate client session
        }
        else {
            send(c.server_socket, c.uprompt, c.bytes_read, 0); // Send command to server
            receive(); // Receive server's response
            fprintf(stdout, "%s", c.server_response); // Print server's response
        }
    }

    halt(EXIT_SUCCESS);
}
