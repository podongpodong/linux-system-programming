#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>
#include <sys/wait.h>
#include <errno.h>

#include "util.h"
#include "hashtable.h"

struct arrange_info {
    char dst_path[PATH_MAX];

    int flag_d;
    int flag_t;
    int flag_x;
    int flag_e;

    char output_path[PATH_MAX];
    int mod_time;
    int exc_cnt;
    char *exc_list[4096];
    int ext_cnt;
    char *ext_list[4096];
};

static struct arrange_info ainfo;
static int dir_token;

static int arrange_option(char **tokens, int ntokens)
{
    int c, ret = 0;
    optind = 1;
    while ((c=getopt(ntokens, tokens, "d:t:x:e:")) != -1) {
        switch(c) {
            case 'd':
                ainfo.flag_d = 1;
                strcpy(ainfo.output_path, optarg);
                break;
            case 't':
                ainfo.flag_t = 1;
                ainfo.mod_time = atoi(optarg);
                break;
            case 'x':
                ainfo.flag_x = 1;
                
                ainfo.exc_list[ainfo.exc_cnt++] = strdup(optarg);
                while (optind < ntokens && tokens[optind][0] != '-') {
                    ainfo.exc_list[ainfo.exc_cnt++] = strdup(tokens[optind]);
                    optind++;
                }
                break;
            case 'e':
                ainfo.flag_e = 1;

                ainfo.ext_list[ainfo.ext_cnt++] = strdup(optarg);
                while (optind < ntokens && tokens[optind][0] != '-') {
                    ainfo.ext_list[ainfo.ext_cnt++] = strdup(tokens[optind]);
                    optind++;
                }
                break;
            case '?':
                ret = -1;
                break;
        }
    }
    dir_token = optind;

    return ret;
}

static void arrange_info_init(void)
{
    memset(ainfo.dst_path, 0, sizeof(ainfo.dst_path));

    ainfo.flag_d = 0;
    ainfo.flag_t = 0;
    ainfo.flag_x = 0;
    ainfo.flag_e = 0;

    memset(ainfo.output_path, 0, sizeof(ainfo.output_path));
    ainfo.mod_time = 0;

    ainfo.exc_cnt = 0;
    ainfo.ext_cnt = 0;

    memset(ainfo.exc_list, 0, sizeof(ainfo.exc_list));
    memset(ainfo.ext_list, 0, sizeof(ainfo.ext_list));
}

//return extenstion if no extenstionn return NULL
char* get_extension(char* file_name) {
	char* extension = strrchr(file_name, '.');

	if(extension == NULL)
		return NULL;
	else
		return extension+1;
}

int exclude_dir(char* dir_name) {

	//related -x option
	//if exclude_path include dir_name, return 1
	for(int i = 0; i<ainfo.exc_cnt; i++) {
		if(strstr(dir_name, ainfo.exc_list[i]) != NULL) 
			return 1;
	}

	return 0;
}

int include_file(char* fname) {
	char* f_extend = get_extension(strdup(fname));
	if(f_extend == NULL)
		f_extend = strdup("NULL");
	for(int i = 0; i<ainfo.ext_cnt; i++) {
		if(strcmp(ainfo.ext_list[i], f_extend) == 0)
			return 0;
	}
	return 1;
}

static void traverse_directory(char *dir_path, char* path)
{
    struct stat file_stat;
    struct dirent **name_list = NULL;
    int name_cnt = 0;

    name_cnt = scandir(dir_path, &name_list, filter_hidden, alphasort);
    
    for (int i = 0; i<name_cnt; i++) {
        char abs_path[PATH_MAX];
        char rel_path[PATH_MAX];

        snprintf(abs_path, PATH_MAX, "%s/%s", dir_path, name_list[i]->d_name);
        snprintf(rel_path, PATH_MAX, "%s/%s", path, name_list[i]->d_name);

        if (stat(abs_path, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                if (ainfo.flag_x == 1 && exclude_dir(abs_path)) continue;
                traverse_directory(abs_path, rel_path);
            }
            else {
                time_t now = time(NULL);

                if (ainfo.flag_t == 1 && now-file_stat.st_mtime > ainfo.mod_time) continue;
                if (ainfo.flag_e == 1 && include_file(name_list[i]->d_name)) continue;

                char *filename = strrchr(abs_path, '/')+1;
                insertFile(filename, rel_path, abs_path);
            }
        }
    }

    for (int i = 0; i<name_cnt; i++)
        free(name_list[i]);
    free(name_list);
}

//exectue vi diff in child process
static void option_proc(char** option) {
	pid_t pid = fork();

	if(pid == 0) {
		execvp(option[0], option);
		exit(1);
	}
	else if(pid > 0){
		wait(NULL);
	}
	else {
		perror("fork fail");
	}
}

//copy file src to dest
static void copy_file(char* src, char* dest) {
	int src_fd, dest_fd;
	ssize_t bytes_read;
	char buffer[4096];

	struct stat src_stat;
	stat(src, &src_stat);

	src_fd = open(src, O_RDONLY);
	dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);

	while((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
		write(dest_fd, buffer, bytes_read);
	}
	
	close(src_fd);
	close(dest_fd);
}

//exist same file
static void option_choosen(HashNode* cur, char* dest_path) {
	//print duplicated file
	//HashNode save file name
	//PathNode save file info about filename
	PathNode* cur_path = cur->path_list;
    while(cur_path != NULL) {
		printf("%d. %s\n", cur_path->idx, cur_path->path);
		cur_path = cur_path->next;
	}
	printf("\n");

	while(1) {
		printf("choose an option:\n");
		printf("0. select [num]\n");	
		printf("1. diff [num1] [num2]\n");
		printf("2. vi [num1]\n");
		printf("3. do not select\n");

		char buf[100]="";
		printf("20211402> "); fflush(stdout);

		fgets(buf, sizeof(buf), stdin);

		buf[strcspn(buf, "\n")] = 0;
		if(strlen(buf) == 0) continue;
		char* copy_buf = strdup(buf);

		char* option[10];
		int i = 0;
		char* token = strtok(copy_buf, " ");
		while(token != NULL) {
			option[i++] = token;
			token = strtok(NULL, " ");
		}
	    option[i]=NULL;

		if(strcmp(buf, "do not select") == 0) {
			break;
		}
		else if(strcmp(option[0], "select") == 0) {
			PathNode* num1 = get_file(cur->path_list, atoi(option[1]));
			//select wrong number about save file
			if(num1 == NULL) continue;
			
			copy_file(num1->abs_path, dest_path);
			break;
		}
		else if(strcmp(option[0], "diff") == 0) {
			PathNode* num1 = get_file(cur->path_list, atoi(option[1]));
			PathNode* num2 = get_file(cur->path_list, atoi(option[2]));

			//select wrong number about duplicated file
			if(num1 == NULL || num2 == NULL) continue;

			option[1] = num1->abs_path;
			option[2] = num2->abs_path;

			//call process function
			option_proc(option);
		}
		else if(strcmp(option[0], "vi") == 0) {
			PathNode* num1 = get_file(cur->path_list, atoi(option[1]));
			option[1] = num1->abs_path;

			//select wrong number about vi file
			if(num1 == NULL) continue;

			//call process function
			option_proc(option);
		}
	}
}

static void arrange_file(void) 
{
	//arrange file
	//read hashtable, if HashNode->cnt > 1, duplicated file exist
    HashNode* cur = ht->buckets;
    while(cur != NULL) {
		char extend_path[PATH_MAX];
		//get extenstion like txt, out, c, h. No extension : NULL
		char* extension = get_extension(cur->filename);
		if(extension == NULL)
			extension = strdup("NULL");

        if (make_path(extend_path, ainfo.dst_path, extension) < 0) {
            perror("make_path");
            continue;
        }
		
		//dest_path 
		char dest_path[PATH_MAX];
        if (make_path(dest_path, extend_path, cur->filename) < 0) {
            perror("make_path");
            continue;
        }

		//no duplicated file
		if(cur->cnt == 1) {
			errno = 0;
			//if dir doesn't exist, make directory, if directory exist, start copy file
			if(mkdir(extend_path, 0775) == 0 || errno == EEXIST) {
				copy_file(cur->path_list->abs_path, dest_path);
			}
		}
		//if duplicated file exist
		else {
			errno = 0;
			if(mkdir(extend_path, 0775) == 0 || errno == EEXIST) {
				option_choosen(cur, dest_path);
			}
    	}
		cur=cur->next;
	}
}

void arrange(char **tokens, int ntokens) {
    int ecode;
    char* expand_path, *copy_path, *target_dir;
    char real_path[PATH_MAX];
    char cwd[PATH_MAX];
    struct stat file_stat;

    arrange_info_init();
    
    if (arrange_option(tokens, ntokens) < 0) {
        perror("option error");
        show_help("arrange");
        return;
    }

    if (tokens[dir_token] == NULL) {
        perror("Not exist path");
        show_help("arrange");
        return;
    }

    if (tokens[dir_token][0] != '/') {
        if ((expand_path = expand_home(tokens[dir_token])) == NULL) {
            perror("Wrong directory path");
            show_help("arrange");
            free(expand_path);
            return;
        }

        if (realpath(expand_path, real_path) == NULL) {
            perror("Wrong directory path");
            show_help("arrange");
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

    if (stat(real_path, &file_stat) < 0) 
        return;

    if (!S_ISDIR(file_stat.st_mode)) {
        printf("%s is not directory\n", tokens[dir_token]);
        return;
    }

    // makdir pwd_arranged or -d <dir_name>
    getcwd(cwd, sizeof(cwd));
    copy_path = strdup(real_path);
    target_dir = basename(copy_path);

    if (ainfo.flag_d) {
        make_path(ainfo.dst_path, cwd, ainfo.output_path);
    }
    else {
        char* arrange_name = strncat(target_dir, "_arranged", 10);
        make_path(ainfo.dst_path, cwd, arrange_name);
    }

    ht = (HashTable*)malloc(sizeof(HashTable));
    ht->buckets = NULL;
    if (mkdir(ainfo.dst_path, 0775) == 0 || errno == EEXIST) {
        // recuersive search
        traverse_directory(real_path, target_dir);
    }

    arrange_file();
    printf("%s arranged\n", tokens[dir_token]);

    freeHashTable();
}
