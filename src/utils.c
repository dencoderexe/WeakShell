#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include "colors.h"

typedef struct DPrompt {
    char* username;
    char hostname[256];
    char hostdate[16];
    char hosttime[16];
    char cwd[1024];
} DPrompt;

DPrompt dprompt;

void default_prompt(void) {
    time_t current_time;
    struct tm *local_time;

    if ((dprompt.username = getlogin()) == NULL) {
        perror(RED "Error: getlogin()" RESET);
        exit(EXIT_FAILURE);
    }
    if ((gethostname(dprompt.hostname, sizeof(dprompt.hostname))) != 0) {
        perror(RED "Error: gethostname()" RESET);
        exit(EXIT_FAILURE);
    }
    if ((current_time = time(NULL)) == -1) {
        perror(RED "Error: time()" RESET);
        exit(EXIT_FAILURE);
    }
    if ((local_time = localtime(&current_time)) == NULL) {
        perror(RED "Error: localtime()" RESET);
        exit(EXIT_FAILURE);
    }
    strftime(dprompt.hostdate, sizeof(dprompt.hostdate), "%d.%m.%Y", local_time);
    strftime(dprompt.hosttime, sizeof(dprompt.hosttime), "%H:%M:%S", local_time);
    if ((getcwd(dprompt.cwd, sizeof(dprompt.cwd))) == NULL) {
        perror(RED "Error: getcwd()" RESET);
        exit(EXIT_FAILURE);
    }
    
    printf(YELLOW "%s|%s|%s@%s|%s" RESET "\n> ", dprompt.hostdate, dprompt.hosttime, dprompt.username, 
                                dprompt.hostname, dprompt.cwd);
}
