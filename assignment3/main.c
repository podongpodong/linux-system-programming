#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "ssu_ext2.h"
#include "util.h"

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

void prompt() {
    char input[4098];
    char **argv = NULL;
	int argc;

	while(1) {
		printf("20211402> ");
		fgets(input, sizeof(input), stdin);
		input[strcspn(input, "\n")] = 0;
		
		if (strlen(input) == 0) continue;
		argc = tokenize_command(input, &argv);

		if (strcmp(argv[0], "exit") == 0) {
            if(argv) free(argv);
			exit(0);
		}
		else if (strcmp(argv[0], "tree") == 0)
			tree(argc, argv);
		else if (strcmp(argv[0], "print") == 0) {
			print_command(argc, argv);
        }
		else if (strcmp(argv[0], "help") == 0)
            help_command(argv[1]);
        else {
            help_command(NULL);
        }

        if (argv) {
            free(argv);
            argv = NULL;
        }
	}
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage Error : ./ssu_ext2 <EXT2_IMAGE>");
        exit(1);
    }

    char* expand_path;
    char real_path[PATH_MAX];

    if (argv[1][0] != '/') {
        if ((expand_path = expand_home(argv[1])) == NULL) {
            perror("Wrong File path");
            free(expand_path);
            return 0;
        }

        if (realpath(expand_path, real_path) == NULL) {
            perror("Wrong directory path");
            
            free(expand_path);
            return 0;
        }
        free(expand_path);
    }
    else
        sprintf(real_path, "%s", argv[1]);

    if (load_ext2_image(real_path) < 0) {
        exit(1);
    }

    prompt();

    ext2_close();
    return 0;
}