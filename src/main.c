// DenCoder.EXE

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

typedef struct Flags {
    bool c;
    bool s;
    bool h;
    bool p;
    bool u;
    bool i;
} Flags;

typedef struct Args
{
    int port;
    char addr[16];
} Args;

static Flags flags;
static Args args;

void help(void) {
    printf(help_header);
    printf(help_launcher);
}

void no_launch_args(void) {
    printf(RED "Error: " RESET "No launch arguments.\n");
    exit(EXIT_FAILURE);
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
        else if (strcmp(argv[i], port_flag) == 0) {
            if ((i+1) < argc && isdigit(argv[i+1][0])) {
                flags.p = true;
                args.port = atoi(argv[i+1]);
            }
            else {
                printf(RED "Error: " RESET "Wrong port format <%s>.\n", argv[i+1]);
                exit(EXIT_FAILURE);
            }
            i++;
        }
        else if (strcmp(argv[i], ip_flag) == 0) {
            if ((i+1) < argc) {
                flags.i = true;
                strcpy(args.addr, argv[i+1]);
            }
            else {
                printf(RED "Error: " RESET "Wrong IP address format <%s>.\n", argv[i+1]);
                exit(EXIT_FAILURE);
            }
            i++;
        }
        // else if (strcmp(argv[i], socket_flag) == 0) {
        //     flags.u = true;
        // }
        else {
            printf(RED "Error: " RESET "Unknown argument <%s>.\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
}

void launch(void) {
    if (flags.h) {
        help();
    }
    else if (flags.s) {
        server(args.port, args.addr);
    }
    else if (flags.c) {
        client(args.port, args.addr);
    }
    else {
        server(args.port, args.addr);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]) {
    args.port = PORT;
    strcpy(args.addr, ADDR);

    parse_args(argc, argv);
    launch();
    exit(EXIT_SUCCESS);

    return 0;
}
