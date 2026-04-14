#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>

#include "util.h"

int filter_hidden(const struct dirent *entry)
{
    return (entry->d_name[0] != '.' || entry->d_name[1] == '_');
}

char *expand_home(const char* path)
{
    const char *home;
    char *result;

    if (path == NULL)
        return NULL;
    
    if (path[0] != '~')
        return strdup(path);

    home = getenv("HOME");
    if (home == NULL)
        return NULL;

    result = (char*)malloc(sizeof(char)*(strlen(home)+strlen(path)));
    if (result == NULL)
        return NULL;

    sprintf(result, "%s/%s", home, path+1);
    return result;
}

int validate_path(const char* path)
{
    const char *home = getenv("HOME");
    char *dup_path, *tok;

    if (path == NULL || home == NULL)
        return -1;

    dup_path = strdup(path);
    tok = strtok(dup_path, "/");
    while (tok != NULL) {
        if (strlen(tok) > NAME_MAX) {
            free(dup_path);
            return -1;
        }
        tok = strtok(NULL, "/");
    }
    free(dup_path);

    if (strncmp(path, home, strlen(home)) != 0) {
        return -2;
    } 
    else if (path[strlen(home)] != '/' && strlen(home) != strlen(path)) {
        return -2;
    }

    return 0;
}

int make_path(char* dst, const char* src1, const char* src2)
{
    int n = snprintf(dst, PATH_MAX, "%s/%s", src1, src2);
    if (n < 0 || (size_t)n >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}
