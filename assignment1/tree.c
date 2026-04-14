#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#include "util.h"

struct tree_info {
    int flag_s;
    int flag_p;
    int dir_cnt;
    int file_cnt;
    int parent_info[4096];
};

static struct tree_info tinfo;
static int dir_token;

static void print_permission(mode_t mode)
{
    char type;
    if (S_ISDIR(mode))
        type = 'd';
    else if (S_ISLNK(mode))
        type = 'l';
    else if (S_ISCHR(mode))
        type = 'c';
    else if (S_ISBLK(mode))
        type = 'b';
    else if (S_ISFIFO(mode))
        type = 'p';
    else if (S_ISSOCK(mode))
        type = 's';
    else
        type = '-';

    printf("%c", type);
	printf("%c", (mode & S_IRUSR) ? 'r' : '-');
	printf("%c", (mode & S_IWUSR) ? 'w' : '-');
	printf("%c", (mode & S_IXUSR) ? 'x' : '-');
	printf("%c", (mode & S_IRGRP) ? 'r' : '-');
	printf("%c", (mode & S_IWGRP) ? 'w' : '-');
	printf("%c", (mode & S_IXGRP) ? 'x' : '-');
	printf("%c", (mode & S_IROTH) ? 'r' : '-');
	printf("%c", (mode & S_IWOTH) ? 'w' : '-');
	printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

static void print_node(char* dir_path, struct stat* file_stat)
{
   if (tinfo.flag_s && tinfo.flag_p) {
       printf("[");
       print_permission(file_stat->st_mode);
       printf(" %ld", file_stat->st_size);
       printf("] ");
   } else if (tinfo.flag_s) {
       printf("[%ld] ", file_stat->st_size);
   } else if (tinfo.flag_p) {
       printf("[");
       print_permission(file_stat->st_mode);
       printf("] ");
   }

   if (S_ISDIR(file_stat->st_mode))
       printf("%s/\n", dir_path);
   else
       printf("%s\n", dir_path);
}

static void print_edge(int depth)
{
    for (int i = 1; i<depth; i++) {
        if (tinfo.parent_info[i])
            printf("    ");
        else
            printf("│   ");
    }

    if(tinfo.parent_info[depth])
		printf("└─ ");
	else
		printf("├─ ");
}

static void print_tree(char* dir_path, struct stat* file_stat, int depth)
{
    print_edge(depth);
    print_node(dir_path, file_stat);
}

static void traverse_directory(const char* dir_path, int depth)
{
    struct dirent **name_list = NULL;
    struct stat file_stat;
    int name_cnt = 0;

    name_cnt = scandir(dir_path, &name_list, filter_hidden, alphasort);

    for (int i = 0; i<name_cnt; i++) {
        char abs_path[PATH_MAX];

        snprintf(abs_path, PATH_MAX, "%s/%s", dir_path, name_list[i]->d_name);
        
        if (lstat(abs_path, &file_stat) == 0) {
            if (i == name_cnt-1)
                tinfo.parent_info[depth] = 1;
            else    
                tinfo.parent_info[depth] = 0;

            if (S_ISDIR(file_stat.st_mode)) {
                tinfo.dir_cnt++;
                print_tree(name_list[i]->d_name, &file_stat, depth);
                traverse_directory(abs_path, depth+1);
            }
            else {
                tinfo.file_cnt++;
                print_tree(name_list[i]->d_name, &file_stat, depth);
            }
        }
    }

    for (int i = 0; i<name_cnt; i++)
        free(name_list[i]);
    free(name_list);
}

static int tree_option(char **tokens, int ntokens)
{
    int c, ret = 0;
    optind = 1;
    while ((c=getopt(ntokens, tokens, "sp")) != -1) {
        switch(c) {
            case 's':
                tinfo.flag_s = 1;
                break;
            case 'p':
                tinfo.flag_p = 1;
                break;
            case '?':
                ret = -1;
                break;
        }
    }
    dir_token = optind;

    return ret;
}

static void tree_info_init(void)
{
    tinfo.flag_s = 0;
    tinfo.flag_p = 0;
    tinfo.dir_cnt = 0;
    tinfo.file_cnt = 0;
    for (int i = 0; i<4096; i++)
        tinfo.parent_info[i] = 0;
}

void run_tree(char **tokens, int ntokens)
{
    int ecode;
    char* expand_path;
    char real_path[PATH_MAX];
    struct stat file_stat;

    tree_info_init();

    if (tree_option(tokens, ntokens) < 0) {
        show_help("tree");
        return;
    }

    if (tokens[dir_token] == NULL) {
        printf("error!\n");
        show_help("tree");
        return;
    }

    if (strlen(tokens[dir_token]) > PATH_MAX) {
        perror("Too long path");
        show_help("tree");
        return;
    }

    // Convert path to realpathval
    if (tokens[dir_token][0] != '/') {
        if ((expand_path = expand_home(tokens[dir_token])) == NULL) {
            perror("Wrong directory path");
            show_help("tree");
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
        sprintf(real_path, "%s", tokens[dir_token]);

    // Check validate
    if ((ecode = validate_path(real_path)) < 0) {
        if (ecode == -2)
            fprintf(stderr, "%s is outside home directory\n", tokens[dir_token]);
        return;
    }

    // excute tree
    if (stat(real_path, &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            tinfo.dir_cnt++;
            print_node(tokens[dir_token], &file_stat);
            traverse_directory(real_path, 1);
            printf("%d directories, %d files\n", tinfo.dir_cnt, tinfo.file_cnt);
        }
        else {
            printf("%s is not directory\n", tokens[dir_token]);
        }
    }
}
