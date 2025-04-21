// Include necessary libraries for system operations, time functions, and utilities
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <string.h>
#include "colors.h"
#include "utils.h"

// Structure to hold user, host, and prompt-related information
typedef struct DPrompt {
    char* username;       // Username of the user
    char hostname[256];   // Hostname of the machine
    char hostdate[11];    // Date in the format "dd.mm.yyyy"
    char hosttime[9];     // Time in the format "hh:mm:ss"
    char cwd[1024];       // Current working directory
    char prompt_buffer[1300]; // Buffer for the complete prompt
} DPrompt;

// Function to fill the DPrompt structure with system information (username, hostname, date, time, cwd)
void default_prompt(DPrompt* dprompt) {
    time_t current_time;
    struct tm *local_time;

    // Get the current username, exit if it fails
    if ((dprompt->username = getlogin()) == NULL) {
        perror(RED "Error: getlogin()" RESET);
        exit(EXIT_FAILURE);
    }
    // Get the current hostname, exit if it fails
    if ((gethostname(dprompt->hostname, sizeof(dprompt->hostname))) != 0) {
        perror(RED "Error: gethostname()" RESET);
        exit(EXIT_FAILURE);
    }
    // Get the current time, exit if it fails
    if ((current_time = time(NULL)) == -1) {
        perror(RED "Error: time()" RESET);
        exit(EXIT_FAILURE);
    }
    // Convert the time to local time, exit if it fails
    if ((local_time = localtime(&current_time)) == NULL) {
        perror(RED "Error: localtime()" RESET);
        exit(EXIT_FAILURE);
    }
    // Format the date and time and store in the prompt structure
    strftime(dprompt->hostdate, sizeof(dprompt->hostdate), "%d.%m.%Y", local_time);
    strftime(dprompt->hosttime, sizeof(dprompt->hosttime), "%H:%M:%S", local_time);
    // Get the current working directory, exit if it fails
    if ((getcwd(dprompt->cwd, sizeof(dprompt->cwd))) == NULL) {
        perror(RED "Error: getcwd()" RESET);
        exit(EXIT_FAILURE);
    }
}

// Function to split a string (`source`) into tokens using the provided delimiters (`delims`)
void get_tokens(char* source, char** tokens, char* delims) {
    char* request_ptr;
    int token_index = 0;
    // Use strtok_r to split the string into tokens safely
    char* token = strtok_r(source, delims, &request_ptr);

    // Loop through the tokens and store them in the tokens array
    while (token != NULL) {
        tokens[token_index] = token;
        token = strtok_r(NULL, delims, &request_ptr);
        token_index++;
    }
    // Mark the end of the tokens array with NULL
    tokens[token_index] = NULL;
}
