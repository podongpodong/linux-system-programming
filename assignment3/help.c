// help.c
#include <stdio.h>
#include <string.h>
#include "header.h"

// 각 명령어의 순수 Usage 내용만 출력
void help_usage_tree_content(void) {
    printf("tree <PATH> [OPTION]... : display the directory structure if <PATH> is a directory\n");
    printf("  -r: display the directory structure recursively if <PATH> is a directory\n");
    printf("  -s: display the directory structure if <PATH> is a directory, including the size of each file\n");
    printf("  -p: display the directory structure if <PATH> is a directory, including the permissions of each\n");
    printf("      directory and file\n");
}

void help_usage_print_content(void) {
    printf("print <PATH> [OPTION]...: print the contents on the standard output if <PATH> is file\n");
    printf("  -n <line_number>: print only the first <line_number> lines of its contents on the standard output if <PATH>\n");
    printf("                      is file\n");
}

void help_usage_help_content(void) {
    printf("help [COMMAND] : show commands for program\n");
}

void help_usage_exit_content(void) {
    printf("exit : exit program\n");
}

// "help" 명령어 또는 알 수 없는 명령어 입력 시 호출
void help_usage_all(void) {
    printf("Usage:\n"); // 전체 Usage의 시작
    printf("> ");
    help_usage_tree_content();
    printf("> ");
    help_usage_print_content();
    printf("> ");
    help_usage_help_content();
    printf("> ");
    help_usage_exit_content();
}

// "help <COMMAND>" 시 호출될 각 명령어별 상세 Usage 출력 함수들
void help_usage_tree(void) {
    printf("Usage: ");
    help_usage_tree_content();
}

void help_usage_print(void) {
    printf("Usage: ");
    help_usage_print_content();
}

void help_usage_help(void) {
    printf("Usage: ");
    help_usage_help_content();
}

void help_usage_exit(void) {
    printf("Usage: ");
    help_usage_exit_content();
}


// help 명령어 처리 메인 함수
void help_command(int argc, char **argv) {
    if (argc == 1) { // "help" 만 입력된 경우
        help_usage_all();
    } else if (argc == 2) { // "help <COMMAND>" 형태
        if (strcmp(argv[1], "tree") == 0) {
            help_usage_tree(); // "Usage: tree..." 형식으로 출력
        } else if (strcmp(argv[1], "print") == 0) {
            help_usage_print(); // "Usage: print..." 형식으로 출력
        } else if (strcmp(argv[1], "help") == 0) {
            help_usage_help(); // "Usage: help..." 형식으로 출력
        } else if (strcmp(argv[1], "exit") == 0) {
            help_usage_exit(); // "Usage: exit..." 형식으로 출력
        } else {
            printf("invalid command '%s'\n\n", argv[1]);
            help_usage_all();
        }
    } else { // 인자가 너무 많은 경우
        help_usage_all();
    }
}
