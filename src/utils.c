#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "utils.h"
#include "colors.h"

// function to change the current working directory
void cd(char* path, DPrompt* d) {
    chdir(d->cwd);
    chdir(path);
    getcwd(d->cwd, sizeof(d->cwd));
}

// function to fill the DPrompt structure with system information (username, hostname, date, time, cwd)
void default_prompt(DPrompt* dprompt) {
    time_t current_time;
    struct tm *local_time;

    // get the current username
    if ((dprompt->username = getlogin()) == NULL) {
        perror(RED "Error: getlogin()" RESET);
        exit(EXIT_FAILURE);
    }
    // get the current hostname
    if ((gethostname(dprompt->hostname, sizeof(dprompt->hostname))) != 0) {
        perror(RED "Error: gethostname()" RESET);
        exit(EXIT_FAILURE);
    }
    // get the current time
    if ((current_time = time(NULL)) == -1) {
        perror(RED "Error: time()" RESET);
        exit(EXIT_FAILURE);
    }
    // convert the time to local time
    if ((local_time = localtime(&current_time)) == NULL) {
        perror(RED "Error: localtime()" RESET);
        exit(EXIT_FAILURE);
    }
    // format the date and time
    strftime(dprompt->hostdate, sizeof(dprompt->hostdate), "%d.%m.%Y", local_time);
    strftime(dprompt->hosttime, sizeof(dprompt->hosttime), "%H:%M:%S", local_time);

    if (strcmp(dprompt->cwd, "") != 0) {
        return;
    }
    
    // get the current working directory
    if ((getcwd(dprompt->cwd, sizeof(dprompt->cwd))) == NULL) {
        perror(RED "Error: getcwd()" RESET);
        exit(EXIT_FAILURE);
    }
}

// function to split a string (`source`) into tokens using the provided delimiters (`delims`)
void get_tokens(char* source, char** tokens, char* delims) {
    char* request_ptr;
    int token_index = 0;
    // use strtok_r to split the string into tokens safely
    char* token = strtok_r(source, delims, &request_ptr);

    // loop through the tokens and store them in the tokens array
    while (token != NULL) {
        tokens[token_index] = token;
        token = strtok_r(NULL, delims, &request_ptr);
        token_index++;
    }
    // mark the end of the tokens array with NULL
    tokens[token_index] = NULL;
}
