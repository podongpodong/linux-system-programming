#include <dirent.h>

#ifndef UTIL_H
#define UTIL_H

char *expand_home(const char* path);
int validate_path(const char* path);
int make_path(char* dst, const char* src1, const char* src2);

#endif
