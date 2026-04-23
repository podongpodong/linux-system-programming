#include <stdio.h>
#include<string.h>
#include<errno.h>
#include<limits.h>

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
	char **argv = NULL;
	int argc;

	while(1) {
		printf("20211402> ");
		fgets(input, sizeof(input), stdin);
		input[strcspn(input, "\n")] = 0;
		
		if (strlen(input) == 0) continue;
		if ((argv = tokenize_command(input, &argc, " \t")) == NULL) {
			continue;
		}

		if (strcmp(argv[0], "exit") == 0) {
            if(argv) free(argv);
			exit(0);
		}
		else if (strcmp(argv[0], "tree") == 0)
			tree(argc, argv);
		else if (strcmp(argv[0], "print") == 0)
			print_command(argc, argv);
		else if (strcmp(argv[0], "help") == 0)
            help_command(argc, argv);
        else {
            help_usage_all();
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

    int ecode;
    char* expand_path;
    char real_path[PATH_MAX];
    struct stat file_stat;

    if (argv[1][0] != '/') {
        if ((expand_path = expand_home(argv[1][0])) == NULL) {
            perror("Wrong File path");
            free(expand_path);
            return;
        }

        if (realpath(expand_path, real_path) == NULL) {
            perror("Wrong directory path");
            show_help("tree");
            free(expand_path);
            return;
        }
        free(expand_path);
    }
    else
        sprintf(real_path, "%s", argv[1]);

    if (load_ext2_image(real_path) < 0) {
        exit(1);
    }

    prompt();

    close(fd);
    exit(0);
}