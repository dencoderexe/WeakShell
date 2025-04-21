// Forward declaration of the DPrompt structure
typedef struct DPrompt DPrompt;

// Function to handle the default prompt logic (takes a pointer to DPrompt)
void default_prompt(DPrompt* dprompt);

// Function to split the input string (`source`) into tokens based on the delimiter(s)
// `tokens` is an array to store the resulting tokens, `delims` are the characters used as delimiters
void get_tokens(char* source, char** tokens, char* delims);
