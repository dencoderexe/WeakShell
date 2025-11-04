// DenCoder.EXE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "define.h"
#include "local.h"
#include "server.h"
#include "client.h"

#define SERVER_MODE     "server"
#define CLIENT_MODE     "client"
#define DEFAULT_MODE    "local"

#define HELP_FLAG   "-h"
#define IP_FLAG     "-i"
#define PORT_FLAG   "-p"
#define SOCKET_FLAG "-u"

typedef enum Modes
{
    HELP,
    LOCAL,
    SERVER,
    CLIENT,
} Modes;

typedef struct Args
{
    Modes mode;
    char* port;
    char* addr;
    char* socket;
} Args;

static Args args;

void help(void) {
    printf("HELP\n");
}

void parse_args(int argc, char* argv[]) {
    if (argc == 1) {
        return;
    }

    for (int i = 1; i < argc ; i++) {
        if (strcmp(argv[i], DEFAULT_MODE) == 0) {
            args.mode = LOCAL;
        }
        else if (strcmp(argv[i], SERVER_MODE) == 0) {
            args.mode = SERVER;
        }
        else if (strcmp(argv[i], CLIENT_MODE) == 0) {
            args.mode = CLIENT;
        }
        else if (strcmp(argv[i], HELP_FLAG) == 0) {
            args.mode = HELP;
        }
        else if (strcmp(argv[i], IP_FLAG) == 0) {
            if ((i+1) < argc) {
                args.addr = argv[i+1];
                i++;
            }
        }
        else if (strcmp(argv[i], PORT_FLAG) == 0) {
            if ((i+1) < argc) {
                args.port = argv[i+1];
                i++;
            }
        }
        else if (strcmp(argv[i], SOCKET_FLAG) == 0) {
            if ((i+1) < argc) {
                args.socket = argv[i+1];
                i++;
            }
        }
    }
}

void launch(void) {
    switch (args.mode)
    {
    case HELP:
        help();
        break;
    
    case LOCAL:
        local();
        break;

    case SERVER:
        server(args.port, args.addr, args.socket);
        break;

    case CLIENT:
        client(args.port, args.addr, args.socket);
        break;

    default:
        break;
    }
}

int main(int argc, char* argv[]) {
    args.port = DEFAULT_PORT;
    args.addr = DEFAULT_ADDR;
    args.mode = LOCAL;

    parse_args(argc, argv);
    launch();

    return 0;
}