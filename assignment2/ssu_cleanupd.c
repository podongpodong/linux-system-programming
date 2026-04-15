#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<limits.h>
#include<fcntl.h>
#include<pwd.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<signal.h>

#include "ssu_cleanupd.h"
#include "show.h"
#include "add.h"
#include "dremove.h"
#include "help.h"

char buffer[5050];

//Parse buffer
char* parse_command[MAX_ARGS];
int parse_count;

//current_deamon_list filedes
FILE* fd_daemon_list;
char* daemon_list_path;

int main(void) {
	signal(SIGCHLD, SIG_IGN);
	init_ssu_cleanupd();
	while (1) {
		//input command
		printf("20211402>");
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = 0;
		
		//Exit the program if the user enters "exit"
		if (strcmp(buffer, "exit") == 0) break;
		//Show the prompt again when Enter is pressed with no input
		if (strlen(buffer) == 0) continue;

		//입력받은 명령어를 잘라낸다.
		tokenize_input(buffer);

		if (strcmp(parse_command[0], "show") == 0) {
			show();
		}
		else if (strcmp(parse_command[0], "add") == 0) {
			add();
		}
		else if (strcmp(parse_command[0], "modify") == 0) {
			modify();
		}
		else if (strcmp(parse_command[0], "remove") == 0) {
			daemon_remove();
		}
		else if (strcmp(parse_command[0], "help") == 0) {
			help(parse_command[1]);
		}
		else {
			help(NULL);
		}	
	}
}

// Initialize ssu_cleanupd: create ~/.ssu_cleanupd and current_deamon_list file if it is not exist
void init_ssu_cleanupd() {
	struct passwd *pw = getpwuid(getuid());
	char path[PATH_MAX];

	//Construct the path: ~/.ssu_cleanupd
	snprintf(path, sizeof(path), "%s/.ssu_cleanupd", pw->pw_dir);

	//Create the ~/.ssu_cleanupd directory if it doesn't exist
	errno = 0;
	if(mkdir(path, 0777) == 0 || errno == EEXIST) {
		//Construct the path: ~/.ssu_cleanupd/current_deamon_list
		snprintf(path, sizeof(path), "%s/.ssu_cleanupd/current_daemon_list", pw->pw_dir);
		
		daemon_list_path = strdup(path);
		//Creat current_deamon_list file if it does not exist
		fd_daemon_list = fopen(path, "w+");
		fclose(fd_daemon_list);
	}
	else {
		fprintf(stderr, "mkdir error\n");
		exit(1);
	}
}

//Tokenizes the input string by space.
//Split combinded short options like "-abc" into separate tokens for getopt() compatibility
//Results stored in parse_commad[]
void tokenize_input(char* buffer) {
		//Tokenize commad following "space"
		char* ptr = strtok(buffer, " ,");
		parse_count = 0;
		while (ptr != 0) {
			//Split combined options like "-spn" into individual flags for getopt()
			if (ptr[0] == '-' && strlen(ptr) > 2) {
				for (int i = 1; i<strlen(ptr); i++) {
					parse_command[parse_count] = (char*)malloc(sizeof(char)*3);
					snprintf(parse_command[parse_count], 3, "-%c", ptr[i]);
					parse_count++;
				}
			}
			else {
				parse_command[parse_count++] = ptr;
			}

			ptr = strtok(NULL, " ,");
		}
	parse_command[parse_count] = NULL;		
}

void convert_tilde(char **path) {
	struct passwd *pw = getpwuid(getuid());
	char convert_path[PATH_MAX];
	
	//문자열의 첫번째 문자가 ~경우 변환 아닌 경우는 그대로 반환
	if((*path)[0] == '~' && ((*path)[1] == '\0' || (*path)[1] == '/'))
		snprintf(convert_path, sizeof(convert_path), "%s%s", pw->pw_dir, *path+1); 
	else
		snprintf(convert_path, sizeof(convert_path), "%s", *path);

	*path = strdup(convert_path);
}
