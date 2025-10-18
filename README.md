# WeakShell

A simple interactive shell in C implementing client-server architecture. Supports both local UNIX domain sockets and TCP/IP sockets (with IP and port specification) for communication. Handles parsing and execution of operators such as | ; > < >> << and uses system calls for process management and I/O redirection.

## TODO
- [ ] Refactor client management: improve data structures and memory handling for multiple clients.
- [ ] Improve message passing logic between server and client for more reliable communication.
- [ ] Rewrite `help` command for clearer instructions and usage examples.
