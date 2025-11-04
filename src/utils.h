#ifndef UTILS_H
#define UTILS_H

// structure for representing the default prompt (username, hostname, etc.)
typedef struct DPrompt {
    char* username;
    char hostname[256];
    char hostdate[11];
    char hosttime[9];
    char cwd[1024];
} DPrompt;

// structure for handling pipe tokens
typedef struct Pipe {
    char* tokens;
} Pipe;

// structure representing a task with its pipes and tokens
typedef struct Task {
    int pipe_count;
    Pipe* pipes;
    char* tokens;
} Task;

void default_prompt(DPrompt* dprompt);
void get_tokens(char* source, char** tokens, char* delims);
void cd(char* path, DPrompt* d);

#endif