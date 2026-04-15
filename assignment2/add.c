#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<fcntl.h>
#include<signal.h>
#include<syslog.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<time.h>
#include<limits.h>
#include<errno.h>

#include "add.h"
#include "ssu_cleanupd.h"
#include "parse_option.h"
#include "arrange.h"
#include "show.h"

void add() {

	//인자의 개수가 부족하면 예외처리
	if( parse_count < 2) {
		fprintf(stderr, "Too less argument\n");
		return;
	}
	//~를 /home/usr로 변경
	convert_tilde(&parse_command[1]);

	char real_dir_path[PATH_MAX];
	//절대경로로 변환
	if (realpath(parse_command[1], real_dir_path) == NULL) {
		fprintf(stderr, "Error : Invalid path\n");
		return;
	}

	//변환한 절대경로가 유효한 디렉터리인지 확인
	if (is_valid_dir(real_dir_path, parse_command[1]) < 0) {
		return;
	}

	//절대경로로 daemon을 생성가능한지 확인
	if (is_valid_monitoring(real_dir_path) < 0) {
		fprintf(stderr, "Error : Already monitoring directory\n");
		return;
	}


	//mkdir <dir_path>_arranged or-d <output_path>
	char new_dir_path[PATH_MAX];
	if (snprintf(new_dir_path, sizeof(new_dir_path), "%s_arranged", real_dir_path) > sizeof(new_dir_path)) {
		fprintf(stderr, "Error: Path length exceeds the maximum limit of 4096 bytes.");
		return;
	}

	//Create ssu_cleanupd.config
	char new_file_path[PATH_MAX];
	if (snprintf(new_file_path, sizeof(new_file_path), "%s/ssu_cleanupd.config", real_dir_path) > sizeof(new_file_path)) {
		fprintf(stderr, "Error: Path length exceeds the maximum limit of 4096 bytes.");
		return;
	}
	int fd = open(new_file_path, O_RDWR | O_CREAT | O_EXCL, 0666);
	if(fd > 0) close(fd);

	//log파일 생성
	char new_log_file_path[PATH_MAX];
	if (snprintf(new_log_file_path, sizeof(new_log_file_path), "%s/ssu_cleanupd.log", real_dir_path) > sizeof(real_dir_path)) {
		fprintf(stderr, "Error: Path length exceeds the maximum limit of 4096 bytes.");
		return;
	}
	fd = open(new_log_file_path, O_RDWR | O_CREAT | O_EXCL, 0666);
	if(fd > 0) close(fd);

	//config파일에 입력할 정보가 담긴 OptionInfo 구조체를 초기값으로 설정
	set_config(real_dir_path, new_dir_path);

	//옵션 파싱함수
	if (parse_option() < 0) {
		return;
	}

	//디몬 프로세스의 작업 시작
	add_daemon(new_file_path);
}

//dir_path가 현재 모니터링중인 디몬 프로세스 목록에 존재하거나
//상위 하위 관계에 있는지를 확인한다.
int is_valid_monitoring(char *dir_path) {
	fd_daemon_list = fopen(daemon_list_path, "r");
    read_daemon_list();
    fclose(fd_daemon_list);

	struct PathNode *cur = head;
    while(cur != NULL) {
		size_t len1 = strlen(cur->path);
		size_t len2 = strlen(dir_path);
		if (strncmp(dir_path, cur->path, len1) == 0) {
			if (dir_path[len1] == '/' || dir_path[len2] == '\0') {
				freePathList();
				return -1;
			}
		}

		if (strncmp(cur->path, dir_path, len2) == 0) {
			if (cur->path[len2] == '/' || cur->path[len2] == '\0') {
				freePathList();
				return -1;
			}
		}

        cur = cur->next;
    }

	freePathList();
	return 0;
}

//config파일에 입려하게될 초기정보를 설정한다.
void set_config(char* real_dir_path, char* new_dir_path) {
	//Init struct
	strcpy(option_init.monitoring_path, real_dir_path);
	option_init.start_time = time(NULL);

	strcpy(option_init.output_path,new_dir_path);

	option_init.time_interval = DEFAULT_TIME;
    option_init.max_log_lines = DEFAULT_MAX_LOG_LINES;

	option_init.extensions[0] = strdup("all");
	option_init.extensions_cnt = 1;

	option_init.exclude_path_cnt = 0;
	option_init.mode_number = 1;

}

//디몬 프로세스를 등록하는 함수
void add_daemon(char* new_file_path) {
	//new Daemon process information write in <DIR_PATH>/ssu_cleanupd.config
	//fork를 한번하게 되는 경우 ssu_cleanupd프로그램 자체가 종료되어 double fork방식 수행
	
	pid_t pid;
	int fd, maxfd;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
	}
	else if(pid > 0)
		return;	
	
	if (setsid() < 0)
		exit(1);

	if ((pid = fork()) < 0) 
		exit(1);
	else if (pid > 0)
		exit(0);

	signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

	maxfd = getdtablesize();
	for (fd = 0; fd < maxfd; fd++)
		close(fd);

	umask(0);
	chdir("/");
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);

	option_init.pid = getpid();
	fd_daemon_list = fopen(daemon_list_path, "a+");
	//write(fd_daemon_list, option_init.monitoring_path, strlen(option_init.monitoring_path));
	fprintf(fd_daemon_list, "%s\n", option_init.monitoring_path);
	fclose(fd_daemon_list);
	write_config(new_file_path, &option_init);

	daemon_task(new_file_path);
	exit(0);
}

//set_config()와 parse_option()에서 설정한 정보를 config파일에 기록
void write_config(char* new_file_path, struct OptionInfo *opInfo) {
	FILE* fp = fopen(new_file_path, "w");

	struct tm *tm_info;
	char time_buf[100];

	tm_info = localtime(&(opInfo->start_time));
	//time_t는 주어진 형식으로 변환
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

	fprintf(fp, "monitoring_path : %s\n", opInfo->monitoring_path);
	fprintf(fp, "pid : %d\n", opInfo->pid);
	fprintf(fp, "start_time : %s\n", time_buf); // time_t는 정수(long)
	fprintf(fp, "output_path : %s\n", opInfo->output_path);
	fprintf(fp, "time_interval : %d\n", opInfo->time_interval);
	if(opInfo->max_log_lines == 0)
		fprintf(fp, "max_log_lines : none\n");
	else
		fprintf(fp, "max_log_lines : %d\n", opInfo->max_log_lines);
	
	if (opInfo->exclude_path_cnt == 0)
		fprintf(fp, "exclude_path : none\n");
	else {	
		fprintf(fp, "exclude_path : ");
		for (int i = 0; i < opInfo->exclude_path_cnt; i++) {
			if (i == opInfo->exclude_path_cnt-1)
    			fprintf(fp, "%s ", opInfo->exclude_path[i]);
			else
    			fprintf(fp, "%s, ", opInfo->exclude_path[i]);
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "extension : ");
	for (int i = 0; i < opInfo->extensions_cnt; i++) {
		if (i == opInfo->extensions_cnt-1)
    		fprintf(fp, "%s", opInfo->extensions[i]);
		else
    		fprintf(fp, "%s, ", opInfo->extensions[i]);
	}
	fprintf(fp, "\n");

	fprintf(fp, "mode : %d\n", opInfo->mode_number);

	fclose(fp);
}

//config파일에 대한 정보를 불러오는 함수
struct OptionInfo* load_config(char* dir_path) {
	struct OptionInfo* newInfo = (struct OptionInfo*)malloc(sizeof(struct OptionInfo));
	FILE* fp;

	int fd = open(dir_path, O_RDONLY);
	if (fd < 0) {
    	perror("open");
	}

	//다른 프로세스에서 write 중이라면 락을 건다.
	struct flock lock;
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_RDLCK;       //읽기 락
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if (fcntl(fd, F_SETLKW, &lock) < 0) {
    	perror("fcntl");
	}
	close(fd);

    // FILE* 변환
    if ((fp = fopen(dir_path, "r")) == NULL) {
        perror("fdopen");
        free(newInfo);
        return NULL;
    }

	//config파일에서 문자열을 잘라내 옵션을 불러온다.
	char buf[PATH_MAX];
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		 buf[strcspn(buf, "\n")] = '\0';
		if (strncmp(buf, "monitoring_path : ", 17) == 0) {
			sscanf(buf+17, "%s", newInfo->monitoring_path);
		}
		else if (strncmp(buf, "output_path : ", 14) == 0) {
			sscanf(buf + 14, "%s", newInfo->output_path);
        }
		else if (strncmp(buf, "pid : ", 6) == 0) {
			int pid = 0;
			sscanf(buf + 6, "%d", &pid);
			newInfo->pid = (pid_t)pid;
		}
		else if (strncmp(buf, "start_time : ", 13) == 0){
			struct tm tm_struct;
			memset(&tm_struct, 0, sizeof(struct tm));
			char time_str[100];
			sscanf(buf + 13, "%d-%d-%d %d:%d:%d", &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday, &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec);
			tm_struct.tm_year -= 1900;  // 년도는 1900부터 시작
    		tm_struct.tm_mon -= 1; //월을 0월 부터시작
			tm_struct.tm_isdst = 0; // 서머타임 무시 (기본 시스템 로컬 시간 사용)
			newInfo->start_time = mktime(&tm_struct);
		}
		else if (strncmp(buf, "time_interval : ", 16) == 0) {
            int time_interval = 0;
            sscanf(buf + 16, "%d", &time_interval);
            newInfo->time_interval = time_interval;
        }
		else if (strncmp(buf, "max_log_lines : ", 16) == 0) {
			char temp[100];
			sscanf(buf + 16, "%s", temp);
		
			int check = -1;

			if((check=atoi(temp)) == 0)
				newInfo->max_log_lines = 0;
			else 
				newInfo->max_log_lines = check;
		}
		else if (strncmp(buf, "exclude_path : ", 15) == 0) {
            newInfo->exclude_path_cnt = 0;
            char *p = buf + 15;
            char *token = strtok(p, " ,");

            while (token != NULL) {
                if (strcmp(token, "none") == 0)
                    break;

                newInfo->exclude_path[newInfo->exclude_path_cnt++] = strdup(token);
                token = strtok(NULL, " ,");
            }
        }
		else if (strncmp(buf, "extension : ", 11) == 0) {
            newInfo->extensions_cnt = 0;
            char *p = buf + 11;
            char *token = strtok(p, " ,");

            while (token != NULL) {
                newInfo->extensions[newInfo->extensions_cnt++] = strdup(token);
                token = strtok(NULL, " ,");
            }
        }
		else if (strncmp(buf, "mode : ", 7) == 0) {
            int mode_number = 0;
            sscanf(buf + 7, "%d", &mode_number);
            newInfo->mode_number = mode_number;
        }
	}

	fclose(fp);
	return newInfo;
}

void daemon_task(char* dir_path) {
	//update option information for interval_time
	while(1) {
		//config파일의 옵션을 불러온다.
		struct OptionInfo* info = load_config(dir_path);

		arrange(info);
		//print_option_info_to_file(info);

		//불러온 시간간격을 설정한다.
		int interval = info->time_interval;
		sleep(interval);
		free(info);
	}
}

//옵션이 잘 기록되었는지 확인하는 디버깅 함수
void print_option_info_to_file(struct OptionInfo* info) {
    FILE *fp = fopen("/home/minuk/project2/v1/daemon_debug.log", "w");

    if (fp == NULL)
        return;

    fprintf(fp, "===== Option Info =====\n");
    fprintf(fp, "monitoring_path : %s\n", info->monitoring_path);
    fprintf(fp, "output_path : %s\n", info->output_path);
    fprintf(fp, "time_interval : %d\n", info->time_interval);

    fprintf(fp, "exclude_path : ");
    if (info->exclude_path_cnt == 0) {
        fprintf(fp, "none");
    }
    else {
        for (int i = 0; i < info->exclude_path_cnt; i++) {
            fprintf(fp, "%s ", info->exclude_path[i]);
        }
    }
    fprintf(fp, "\n");

    fprintf(fp, "extension : ");
    if (info->extensions_cnt == 0) {
        fprintf(fp, "none");
    }
    else {
        for (int i = 0; i < info->extensions_cnt; i++) {
            fprintf(fp, "%s ", info->extensions[i]);
        }
    }
    fprintf(fp, "\n");

    fprintf(fp, "mode : %d\n", info->mode_number);
    fprintf(fp, "========================\n");

    fclose(fp);
}

