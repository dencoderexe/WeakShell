// DenCoder.EXE

// Assignment #2
// Author: Denis Danilov
// Assignment text:
    // Napíšte v jazyku C jednoduchý interaktívny program, "shell", ktorý bude opakovane čakať na
    // zadanie príkazu a potom ho spracuje. Na základe princípov klient-server architektúry tak musí
    // s pomocou argumentov umožňovať funkciu servera aj klienta. Program musí umožňovať
    // spúšťať zadané príkazy a bude tiež interpretovať aspoň 4 z nasledujúcich špeciálnych
    // znakov: # ; < > | \ . Príkazy musí byť možné zadať zo štandardného vstupu a tiež zo spojení
    // reprezentovaných soketmi. Na príkazovom riadku musí byť možné špecifikovať prepínačom -
    // p port číslo portu a/alebo prepínačom -u cesta názov lokálneho soketu na ktorých bude
    // program čakať na prichádzajúce spojenia. Po spustení s prepínačom -h sa musia vypísať
    // informácie o autorovi, účele a použití programu, zoznam príkazov. "Shell" musí poskytovať
    // aspoň nasledujúce interné príkazy: help - výpis informácií ako pri -h, quit - ukončenie
    // spojenia z ktorého príkaz prišiel, halt - ukončenie celého programu.
    // Prednastavený prompt musí pozostávať z mena používateľa, názvu stroja, aktuálneho času a
    // zvoleného ukončovacieho znaku, e.g. '16:34 user17@student#'. Na zistenie týchto informácií
    // použite vhodné systémové volania s použitím knižničných funkcií. Na formátovanie výstupu,
    // zistenie mena používateľa z UID a pod. môžte v programe využiť bežné knižničné funkcie.
    // Spúšťanie príkazov a presmerovanie súborov musia byť implementované pomocou príslušných
    // systémových volaní. Tie nemusia byť urobené priamo (cez asembler), avšak knižničná
    // funkcia popen(), prípadne podobná, nesmie byť použitá. Pri spustení programu bez
    // argumentov, alebo s argumentom "-s" sa program bude správať vyššie uvedeným spôsobom,
    // teda ako server. S prepínačom "-c" sa bude správať ako klient, teda program nadviaže
    // spojenie so serverom cez socket, do ktorého bude posielať svoj štandardný vstup a čítať dáta
    // pre výstup. Chybové stavy ošetrite bežným spôsobom. Počas vytvárania programu (najmä
    // kompilácie) sa nesmú zobrazovať žiadne varovania a to ani pri zadanom prepínači
    // prekladača -Wall.
    // Vo voliteľných častiach zadania sa očakáva, že tie úlohy budú mať vaše vlastné riešenia, nie
    // jednoduché volania OS.
// Deadline: 20.04.2025, 23:59
// Second year, 2024/2025, summer semester, computer science

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

// Self-evaluation:
    // The program is in a working state and represents a simple version of shell using system calls to perform tasks. 
    // The user can select the client or server version of the program at initial startup using command line flags, and connect 
    // according to the specified parameters. The parameters can be a port, an IP address or a local UNIX socket. In case the user 
    // does not specify any data, the server and client will by default listen and try to connect to the address 127.0.0.1:50000.

    // The program is written in C using UNIX (Linux) system calls and libraries and runs on linux systems (FreeBSD has not been tested). 
    // A Makefile is written to compile the program, which can be used to quickly and conveniently compile an entire project.

    // Unfortunately I did not succeed in implementing the abort n function, which could allow to disconnect the user from the server, 
    // as well as did not manage to fully implement the cd function, it works fine in the case of the client (the request comes from the client), 
    // but does not work with requests coming from the server (cli mode). 

    // Otherwise, the program should behave stably, during my testing, except for the above described problems, I did not encounter anything. 
    // I believe that the work is done at a decent level and is a good start for the realization of a full-formed project, which I would like 
    // to continue when I have free time, I liked the task.
// Completed bonus tasks:
    // 3. (3 body) Interný príkaz stat vypíše zoznam všetkých aktuálnych spojení na ktorých
    // prijíma príkazy, prípadne aj všetky sokety na ktorých prijíma nové spojenia.
    // 7. (2 body) S prepínačom "-i" bude možné zadať aj IP adresu na ktorej bude program
    // očakávať spojenia (nielen port).
    // 11. (2 body) Jeden z príkazov bude využívať funkcie implementované v samostatnej
    // knižnici, ktorá bude "prilinkovaná" k hlavnému programu.
    // 21. (2 body) Funkčný Makefile.
    // 23. (1 bod) Dobré komentáre, resp. rozšírená dokumentácia, v anglickom jazyku.