// DenCoder.EXE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "flags.h"
#include "help.h"
#include "colors.h"
#include "utils.h"
#include "server.h"
#include "client.h"

typedef struct Flags {
    bool c;
    bool s;
    bool h;
    bool p;
    bool u;
} Flags;

Flags flags;

void help(void) {
    printf(help_header);
    printf(help_launcher);
}

void no_launch_args(void) {
    printf(RED "Error: " RESET "No launch arguments.\n");
}

void parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], server_flag) == 0) {
            flags.s = true;
        }
        else if (strcmp(argv[i], client_flag) == 0) {
            flags.c = true;
        }
        else if (strcmp(argv[i], help_flag) == 0) {
            flags.h = true;
        }
        else if (strcmp(argv[i], port_flag) == 0) { //
            flags.p = true;
        }
        else if (strcmp(argv[i], socket_flag) == 0) { //
            flags.u = true;
        }
        else {
            printf(RED "Error: " RESET "Unknown argument <%s>.\n", argv[i]);
        }
    }
}

int launch(void) {
    if (flags.h) {
        help();
        return EXIT_SUCCESS;
    }
    else if (flags.s) {
        return server();
    }
    else if (flags.c) {
        return client();
    }
    else {
        no_launch_args();
        return EXIT_FAILURE;
    }
}

int main(int argc, char* argv[]) {  
    if (argc == 1) {
        return server();
    }
    else {
        parse_args(argc, argv);
        return launch();
    }
}
