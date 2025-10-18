// DenCoder.EXE

// Include necessary libraries for various functions and settings
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "flags.h"
#include "help.h"
#include "colors.h"
#include "utils.h"
#include "server.h"
#include "client.h"
#include "default.h"

// Structures for flags and launch arguments
typedef struct Flags {
    bool c;  // Flag for client
    bool s;  // Flag for server
    bool h;  // Flag for help
} Flags;

typedef struct Args
{
    char* port;           // Port for connection
    char* addr;           // IP address or hostname
    char* socket_path;    // Path to socket
} Args;

// Static variables to store flags and arguments
static Flags flags;
static Args args;

// Function to display help information
void help(void) {
    printf(help_header);  // Print help header
    printf(help_launcher); // Print help content
}

// Function to handle the error when no launch arguments are provided
void no_launch_args(void) {
    printf(RED "Error: " RESET "No launch arguments.\n"); // Error message for missing arguments
    exit(EXIT_FAILURE);  // Exit the program with an error code
}

// Function to parse command-line arguments
void parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        // Check for server flag
        if (strcmp(argv[i], server_flag) == 0) {
            flags.s = true;
        }
        // Check for client flag
        else if (strcmp(argv[i], client_flag) == 0) {
            flags.c = true;
        }
        // Check for help flag
        else if (strcmp(argv[i], help_flag) == 0) {
            flags.h = true;
        }
        // Check for port flag and validate format
        else if (strcmp(argv[i], port_flag) == 0) {
            if ((i+1) < argc && isdigit(argv[i+1][0])) {
                args.port = argv[i+1]; // Store port
            }
            else {
                printf(RED "Error: " RESET "Wrong port format <%s>.\n", argv[i+1]); // Invalid port format
                exit(EXIT_FAILURE);
            }
            i++;
        }
        // Check for IP address flag and store the address
        else if (strcmp(argv[i], ip_flag) == 0) {
            if ((i+1) < argc) {
                args.addr = argv[i+1];
            }
            else {
                printf(RED "Error: " RESET "Wrong IP address format <%s>.\n", argv[i+1]); // Invalid IP address format
                exit(EXIT_FAILURE);
            }
            i++;
        }
        // Check for socket path flag and store the path
        else if (strcmp(argv[i], socket_flag) == 0) {
            if ((i+1) < argc) {
                args.socket_path = argv[i+1];
            }
            else {
                printf(RED "Error: " RESET "Wrong socket PATH format <%s>.\n", argv[i+1]); // Invalid socket path format
                exit(EXIT_FAILURE);
            }
            i++;
        }
        // Handle unknown argument
        else {
            printf(RED "Error: " RESET "Unknown argument <%s>.\n", argv[i]); // Unknown argument error
            exit(EXIT_FAILURE);
        }
    }
}

// Function to launch the appropriate server or client based on flags
void launch(void) {
    if (flags.h) {
        help(); // Show help if flag is set
    }
    else if (flags.s) {
        server(args.port, args.addr, args.socket_path); // Launch server with provided arguments
    }
    else if (flags.c) {
        client(args.port, args.addr, args.socket_path); // Launch client with provided arguments
    }
    else {
        server(args.port, args.addr, args.socket_path); // Default to server if no flags are set
    }
}

// Main function to process arguments and launch the application
int main(int argc, char* argv[]) {
    args.port = PORT;  // Default port
    args.addr = ADDR;  // Default address

    parse_args(argc, argv); // Parse the command-line arguments
    launch(); // Launch the appropriate service (server or client)
    exit(EXIT_SUCCESS); // Exit the program successfully

    return 0; // Return code (though unreachable due to exit)
}