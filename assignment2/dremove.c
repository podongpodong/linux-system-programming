#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<signal.h>
#include<limits.h>

#include "ssu_cleanupd.h"
#include "show.h"
#include "dremove.h"
#include "parse_option.h"

void daemon_remove() {

	if (parse_count != 2) {
		fprintf(stderr, "Error: remove <dir_path>\n");
		return;
	}
	//~을 /home/usr로 변환
	convert_tilde(&parse_command[1]);

	//current_daemon_list에서 실행중인 디몬프로세스 목록을 읽어온다.
	fd_daemon_list = fopen(daemon_list_path, "r");
	read_daemon_list();
	fclose(fd_daemon_list);
	
	//경로가 유효하지 않은 경우 에러처리
	char real_path[PATH_MAX];
	if (realpath(parse_command[1], real_path) == NULL) {
		fprintf(stderr, "Invalid path : <%s>\n", parse_command[1]);
		freePathList();
		return;
	}
	if (is_valid_dir(real_path, parse_command[1]) < 0) {
		return;
	}

	//불러온 디몬 목록에 remove의 인자와 같은의미가 존재한다면 통과
	struct PathNode* cur = head;
	int isExist = 0;
	while(cur != NULL) {
		if(strcmp(cur->path, real_path) == 0) {
			isExist = 1;
			break;
		}
		cur = cur->next;
	}

	if (isExist == 0) {
		fprintf(stderr, "Error: Not current monitoring path\n");
		freePathList();
		return;
	}
	
	char file_path[5000];
    snprintf(file_path, sizeof(file_path), "%s/ssu_cleanupd.config", cur->path);

	FILE* fp = fopen(file_path, "r");
	if(fp == NULL) printf("fp error\n");
	char lineBuf[5000];
	char* str_pid;
	//config파일에서 pid부분만 불러온다.
    while(fgets(lineBuf, sizeof(lineBuf), fp) != NULL) {
		if((str_pid = strstr(lineBuf, "pid : ")) != NULL) {
			str_pid += strlen("pid : ");
			break;
		}
    }

	pid_t pid = (pid_t)atol(str_pid);
	kill(pid, SIGKILL);

	//디몬 프로세스를 종료한 후 current_daemon_list를 수정한다.
	fd_daemon_list = fopen(daemon_list_path, "w+");
	struct PathNode* temp = head;
	while(temp != NULL) {
		if(strcmp(temp->path, cur->path) != 0)
			fprintf(fd_daemon_list, "%s\n", temp->path);
		temp = temp->next;
	}

	freePathList();
	fclose(fd_daemon_list);
}
