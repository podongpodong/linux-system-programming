#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<limits.h>

#include "ssu_cleanupd.h"
#include "parse_option.h"
#include "add.h"
#include "show.h"

void modify() {

	if (parse_count < 2) {
        fprintf(stderr, "Too less argument\n");
        return;
    }

	//~을 /home/usr로 변환
	convert_tilde(&parse_command[1]);

	//디렉터리가 유효한지 확인
	char real_path[PATH_MAX];
	if (realpath(parse_command[1], real_path) == NULL) {
		return;
	}

	if (is_valid_dir(real_path, parse_command[1]) < 0) {
        return;
    }

	//current_daemon_list에서 디몬 목록을 불러와 유효한 인자 입력인지 확인
	fd_daemon_list = fopen(daemon_list_path, "r");
    read_daemon_list();
    fclose(fd_daemon_list);

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
	else
		freePathList();

	char file_path[PATH_MAX];
	if (snprintf(file_path, sizeof(file_path), "%s/ssu_cleanupd.config", real_path) > sizeof(file_path)) {
		fprintf(stderr, "Error: Path length exceeds the maximum limit of 4096 bytes.");
        return;
	}

	//설정 파일을 불러온다.
	struct OptionInfo *info = load_config(file_path);

	// 옵션 파싱
   	if ( parse_option() < 0) {
		return;
   	}

    // 기존 OptionInfo에 입력된 옵션 반영
    if (flag_d)
        strcpy(info->output_path, option_init.output_path);

    if (flag_i)
        info->time_interval = option_init.time_interval;

    if (flag_l)
        info->max_log_lines = option_init.max_log_lines;

    if (flag_x) {
        for (int i = 0; i < info->exclude_path_cnt; i++) {
            free(info->exclude_path[i]);
            info->exclude_path[i] = NULL;
        }
        info->exclude_path_cnt = option_init.exclude_path_cnt;
        for (int i = 0; i < option_init.exclude_path_cnt; i++) {
            info->exclude_path[i] = strdup(option_init.exclude_path[i]);
        }
    }

    if (flag_e) {
        for (int i = 0; i < info->extensions_cnt; i++) {
            free(info->extensions[i]);
            info->extensions[i] = NULL;
        }
        info->extensions_cnt = option_init.extensions_cnt;
        for (int i = 0; i < option_init.extensions_cnt; i++) {
            info->extensions[i] = strdup(option_init.extensions[i]);
        }
    }

    if (flag_m)
        info->mode_number = option_init.mode_number;

	int fd;
	struct flock lock;

	if ((fd = open(file_path, O_RDWR)) < 0) {
	}

	//config파일에 대해서 쓰기 잠금 설정
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if (fcntl(fd, F_SETLKW, &lock) == -1) {
	}

	//설정파일 업데이트
	write_config(file_path, info);

	//파일 잠금해제
	lock.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLK, &lock) == -1) {
	}

	close(fd);
}
