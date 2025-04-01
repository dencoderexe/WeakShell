#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include "colors.h"
#include "help.h"
#include "utils.h"

static int server_socket;
static struct sockaddr_in server_address;
static char server_response[1024];
static char uprompt[1024];

int connect_to_server(void) {
    // create socket, AF_INET - IPv4, SOCK_STREAM - TCP, 0 - IP
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // specify an address for the socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if(connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        perror(RED "Error: connect()" RESET);
        close(server_socket);
        return -1;
    }
    else {
        fprintf(stdout, GREEN "Connection established.\n" RESET);
        return 0;
    }
}

static int halt(void) {
    printf(CYAN "Disconnected.\n" RESET);
    close(server_socket);
    return EXIT_SUCCESS;
}

static void help(void) {
    printf(help_header);
    printf(help_client);
}

int client(void) {    
    if (connect_to_server() != 0) {
        return EXIT_FAILURE;
    }

    while(true) {
        default_prompt();
        if (fgets(uprompt, sizeof(uprompt), stdin)) {
            uprompt[strcspn(uprompt, "\n")] = '\0';
        }

        if (strcmp(uprompt, "help") == 0) {
            help();
        }
        else if (strcmp(uprompt, "halt") == 0) {
            return halt();
        }
        else {
            send(server_socket, uprompt, strlen(uprompt), 0);
            memset(uprompt, 0, sizeof(uprompt));  // Clear buffer
            ssize_t bytes_received = recv(server_socket, &server_response, sizeof(server_response) - 1, 0);
            if (bytes_received <= 0) {
                // If recv() returns 0, the server disconnected
                if (bytes_received == 0) {
                    printf(RED "Server disconnected.\n" RESET);
                } 
                // If recv() returns -1, an error occurred
                else {
                    perror(RED "Error: recv()" RESET);
                }
                close(server_socket);
                return EXIT_FAILURE;
            }
            fprintf(stdout, "%s\n", server_response);
        }
    }
}