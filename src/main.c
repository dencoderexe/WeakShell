// DenCoder.EXE

#include "headers.h"

typedef struct Flags {
    bool c;
    bool s;
    bool d;
    bool l;
    bool h;
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
        else if (strcmp(argv[i], daemon_flag) == 0) {
            flags.d = true;
        }
        else if (strcmp(argv[i], log_flag) == 0) {
            flags.l = true;
        }
        else if (strcmp(argv[i], help_flag) == 0) {
            flags.h = true;
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
        return server(flags.l, flags.d);
    }
    else if (flags.c) {
        if (flags.d) {
            printf(RED "Error: " RESET "client cannot be launched in daemon mode.\n");
        }
        return client(flags.l);
    }
    else {
        no_launch_args();
        return EXIT_FAILURE;
    }
}

int main(int argc, char* argv[]) {  
    if (argc == 1) {
        no_launch_args();
        return EXIT_FAILURE;
    }
    else {
        parse_args(argc, argv);
    }
    
    return launch();
}
