// Define the help message header for the shell version and author
#define help_header     "WeakShell version 0.01, (c) 2025 Denis Danilov\n"
// Define the help message for the launcher commands with usage instructions
#define help_launcher   "Launcher commands:\n" \
                        "   weakshell -h\t\t\thelp message\n" \
                        "   weakshell -c \t\tlaunch in client mode\n" \
                        "   weakshell or weakshell -s \tlaunch in server mode\n" \
                        "\t\t\t\tuse -u [path] \tset local socket path\n" \
                        "\t\t\t\tuse -i [ip addr] \tset socket ip address\n" \
                        "\t\t\t\tuse -p [port]] \tset socket port\n"
// Define the help message for server commands and their functions
#define help_server     "Server commands:\n" \
                        "   help\t\t\thelp message\n" \
                        "   halt\t\t\tshut server down\n" \
                        "   stat\t\t\tdisplay active sockets and connections\n"
// Define the help message for client commands and their functions
#define help_client     "Client commands:\n" \
                        "   help\t\t\thelp message\n" \
                        "   quit\t\t\tdisconnect, shut client down\n" \
                        "   halt\t\t\tshut server and client down\n" \
                        "   stat\t\t\tdisplay active sockets and connections on server\n" \
                        "   cd\t\t\tchange working directory\n"
