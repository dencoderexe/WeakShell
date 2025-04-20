typedef struct DPrompt DPrompt;

void default_prompt(DPrompt* dprompt);

void get_tokens(char* source, char** tokens, char* delims);
