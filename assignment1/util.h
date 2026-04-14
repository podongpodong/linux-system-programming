#include <dirent.h>

#ifndef UTIL_H
#define UTIL_H

// tree
int filter_hidden(const struct dirent *entry);
void run_tree(char** tokens, int ntokens);

// arrange
void arrange(char** tokens, int ntokens);

// help
void show_help(char *command);

char *expand_home(const char* path);
int validate_path(const char* path);
int make_path(char* dst, const char* src1, const char* src2);

#endif
