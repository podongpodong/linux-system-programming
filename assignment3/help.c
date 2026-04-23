// help.c
#include <stdio.h>
#include <string.h>
#include "util.h"

// 각 명령어의 순수 Usage 내용만 출력
static void help_usage_tree(void) {
    printf("tree <PATH> [OPTION]... : display the directory structure if <PATH> is a directory\n");
    printf("  -r: display the directory structure recursively if <PATH> is a directory\n");
    printf("  -s: display the directory structure if <PATH> is a directory, including the size of each file\n");
    printf("  -p: display the directory structure if <PATH> is a directory, including the permissions of each\n");
    printf("      directory and file\n");
}

static void help_usage_print(void) {
    printf("print <PATH> [OPTION]...: print the contents on the standard output if <PATH> is file\n");
    printf("  -n <line_number>: print only the first <line_number> lines of its contents on the standard output if <PATH>\n");
    printf("                      is file\n");
}

static void help_usage_help(void) {
    printf("help [COMMAND] : show commands for program\n");
}

static void help_usage_exit(void) {
    printf("exit : exit program\n");
}

// "help" 명령어 또는 알 수 없는 명령어 입력 시 호출
static void help_usage_all(void) {
    printf("Usage:\n"); // 전체 Usage의 시작
    printf("> ");
    help_usage_tree();
    printf("> ");
    help_usage_print();
    printf("> ");
    help_usage_help();
    printf("> ");
    help_usage_exit();
}

void help_command(const char *cmd) {
    if (cmd == NULL || strlen(cmd) == 0) {
        help_usage_all();
        return;
    }

    if (strcmp(cmd, "tree") == 0) {
        printf("Usage: ");
        help_usage_tree();
    } else if (strcmp(cmd, "print") == 0) {
        printf("Usage: ");
        help_usage_print();
    } else if (strcmp(cmd, "help") == 0) {
        printf("Usage: ");
        help_usage_help();
    } else if (strcmp(cmd, "exit") == 0) {
        printf("Usage: ");
        help_usage_exit();
    } else if (cmd == NULL) {
        help_usage_all();
    } else {
        printf("invalid command '%s'\n\n", cmd);
        help_usage_all();
    }
}
