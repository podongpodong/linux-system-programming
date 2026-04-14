#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define COMMAND_TOKEN 0

int tokenize_command(char* buf, char ***tokens)
{
    int capacity = 16, ntokens = 0;
    *tokens = (char**)malloc(sizeof(char*)*capacity);

    char *tok = strtok(buf, " ");
    while (tok != NULL) {
        if (ntokens >= capacity) {
            capacity *= 2;
            *tokens = (char**)realloc(*tokens, sizeof(char*)*capacity);
        }
        (*tokens)[ntokens] = tok;
        ntokens+=1; 
        tok = strtok(NULL, " ");
    }
    (*tokens)[ntokens] = NULL;

    return ntokens;
}

int main(void)
{
    char buffer[4096];
    char **tokens = NULL;
    int ntokens;
    while(1) {
        printf("20211402> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) == 0) continue;

        ntokens = tokenize_command(buffer, &tokens);

        if (strcmp(tokens[COMMAND_TOKEN], "tree") == 0 && ntokens >= 2) {
            run_tree(tokens, ntokens);
        }
        else if (strcmp(tokens[COMMAND_TOKEN], "arrange") == 0 && ntokens >= 2) {
            arrange(tokens, ntokens);
        }
        else if (strcmp(tokens[COMMAND_TOKEN], "help") == 0) {
            show_help(tokens[COMMAND_TOKEN+1]);
        }
        else if (strcmp(tokens[COMMAND_TOKEN], "exit") == 0) {
            // program exit
            break;
        } else {
            show_help(NULL);
        }

    }
    return 0;
}
