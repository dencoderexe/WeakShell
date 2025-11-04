#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "colors.h"

#define True 1
#define False 0

DPrompt dprompt;
int bytes_read;
size_t uprompt_len;
char* uprompt;

void print_dprompt(void) {
    default_prompt(&dprompt);
    printf(YELLOW "%s|%s|%s@%s|%s" RESET "\n> ", dprompt.hostdate, dprompt.hosttime, dprompt.username, dprompt.hostname, dprompt.cwd);
    fflush(stdout);
}

void get_user_prompt(void) {
    if ((bytes_read = getline(&uprompt, &uprompt_len, stdin)) == -1) {
        perror(RED "Error: getline()" RESET);
        exit(EXIT_FAILURE);
    }
    uprompt[bytes_read-1] = '\0';
}

int local(void) {
    while (True)
    {
        print_dprompt();
        get_user_prompt();
    }
    
    return 0;
}