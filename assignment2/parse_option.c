#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<pwd.h>
#include<sys/stat.h>

#include "ssu_cleanupd.h"
#include "parse_option.h"
#include "show.h"

const int DEFAULT_TIME = 10;
const int DEFAULT_MAX_LOG_LINES = 0;
const int DEFAULT_MODE = 1;

struct OptionInfo option_init;

int flag_d;
int flag_i;
int flag_l;
int flag_x;
int flag_e;
int flag_m;

int parse_option() {

	optind = 1;
	opterr = 0;
	int c;

	//-i -l -m : must be digit
	
	flag_d = 0;
	flag_i = 0;
	flag_l = 0;
	flag_x = 0;
	flag_e = 0;
	flag_m = 0;

	while((c=getopt(parse_count, parse_command, "d:i:l:x:e:m:")) != -1) {
		switch(c) {
			case 'd' :
				if (optarg == NULL || optarg[0] == '-') {
                	fprintf(stderr, "Missing argument for -d option\n");
                	return -1;
            	}
				flag_d = 1;

				//~을 /home/usr로 변환
				convert_tilde(&optarg);

				char abs_path[PATH_MAX];
				//디렉터리가 유효한지 확인
				if (realpath(optarg, abs_path) == NULL) {
					fprintf(stderr, "Invalid path : <%s>\n", optarg);
					return -1;
				}
				if (is_valid_dir(abs_path, optarg) < 0) {
					return -1;
				}
				//-d 옵션 인자에 대해서 첫번째 인자의 경로를 포함하는지 확인
				if (is_valid_output(parse_command[1], abs_path) < 0) {
					fprintf(stderr, "Error : <%s> is included in <%s>\n", optarg, parse_command[1]);
					return -1;
				}

				strcpy(option_init.output_path,abs_path);

				break;
			case 'i' :
				if (optarg == NULL || optarg[0] == '-') {
                	fprintf(stderr, "Missing argument for -i option\n");
                	return -1;
            	}
				flag_i = 1;
				int interval;
				
				//자연수 입력인지 확인한다.
				if (isNumber(optarg) < 0 || (interval = atoi(optarg)) == 0) {
					fprintf(stderr, "Invalid input: must be a natural number\n");
					return -1;
				}

				option_init.time_interval = interval;
				break;
			case 'l' :
				if (optarg == NULL || optarg[0] == '-') {
                	fprintf(stderr, "Missing argument for -l option\n");
                	return -1;
            	}
				flag_l = 1;
				
				int log_lines;
				//자연수 입력인지 확인한다.
				if (isNumber(optarg) < 0 || (log_lines = atoi(optarg)) == 0) {
					fprintf(stderr, "Invalid input: must be a natural number\n");
					return -1;
				}

				option_init.max_log_lines = log_lines;
				break;
			case 'x' :
				if (optarg == NULL) {
                	fprintf(stderr, "Missing argument(s) for -x option\n");
                	return -1;
            	}
				flag_x = 1;
				option_init.exclude_path_cnt = 0;
				for (int i = optind-1; i<parse_count; i++) {
					if (parse_command[i][0] == '-') {
						break;
					}

					//~을 /home/usr로 변환한다.
					convert_tilde(&parse_command[i]);
					
					char abs_path[PATH_MAX];
					//디렉터리가 유효한지 확인한다.
					if (realpath(parse_command[i], abs_path) == NULL) {
						fprintf(stderr, "Invalid path : <%s>\n", parse_command[i]);
						return -1;
					}
					if (is_valid_dir(abs_path, parse_command[i]) < 0) {
						return -1;
					}

					//-e옵션 인자들간에 상위, 하위, 포함관계인지 확인
					//-e옵션이 첫번째 인자의 하위에 존재하는지 확인
					if (is_valid_exclude(&option_init, abs_path) < 0) {
						return -1;
					}

					option_init.exclude_path[option_init.exclude_path_cnt++] = strdup(abs_path);
				}
				optind--;
				break;
			case 'e' :
				if (optarg == NULL) {
                	fprintf(stderr, "Missing argument(s) for -e option\n");
                	return -1;
            	}
				flag_e = 1;
				free(option_init.extensions[0]);
				option_init.extensions_cnt = 0;
				//확장자 목록을 저장
				for (int i = optind-1; i<parse_count; i++) {
					if (parse_command[i][0] == '-') {
						break;
					}
					option_init.extensions[option_init.extensions_cnt++] = strdup(parse_command[i]);
				}
				optind--;
				break;
			case 'm' :
				if (optarg == NULL || optarg[0] == '-') {
               	 	fprintf(stderr, "Missing argument for -m option\n");
                	return -1;
            	}
				flag_m = 1;
				//자연수가 입력되었는지 확인
				int mode_number;
				if (isNumber(optarg) < 0 || (mode_number = atoi(optarg)) == 0) {
					fprintf(stderr, "Invalid input: must be a natural number\n");
					return -1;
				}

				//1~3범위의 입력인지 확인
				if (mode_number > 3) {
					fprintf(stderr, "Error: mode must be between 1 and 3\n");
					return -1;
				}

				option_init.mode_number = atoi(optarg);
				break;
			case '?' :
				fprintf(stderr, "Unknown option or missing argument\n");
				return -1;
				break;
		}
	}

	return 0;
}

int is_valid_dir(char* abs_path, char* dir_path) {
	struct stat statbuf;
	struct passwd *pw;

	//디렉터리가 아닌 경우 확인
	if (lstat(abs_path, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
		fprintf(stderr, "Not a valid directory : <%s>\n", dir_path);
		return -1;
	}

	//해당 경로에 접근 권한이 없는 경우
	if (access(abs_path, R_OK|X_OK) != 0) {
		fprintf(stderr, "No permission to access <%s>\n", dir_path);
		return -1;
	}

	pw = getpwuid(getuid());

	//사용자의 홈 디렉터리를 벗어나는 경우
	if (strncmp(abs_path, pw->pw_dir, strlen(pw->pw_dir)) != 0) {
		fprintf(stderr, "<%s> is outside the home directory\n", dir_path);
		return -1;
	}

	return 0;
}

int is_valid_output(char *dir_path, char *output) {
	size_t len = strlen(dir_path);
	if (strncmp(output, dir_path, len) == 0) {
		if(output[len] == '/' || output[len] == '\0') {
			return -1;
		}
	}
	return 0;
}

int is_valid_exclude(struct OptionInfo *info, char *exclude) {
	size_t len = strlen(info->monitoring_path);
	//첫번째 인자의 하위 경로에 위치하지 않는 경우 예외처리
	if (strncmp(exclude, info->monitoring_path, len) == 0) {
		if (exclude[len] != '/') {
			fprintf(stderr, "Error : Invalid Exclude path\n");
			return -1;
		}
	}
	else {
		fprintf(stderr, "Error : Invalid Exclude path\n");
		return -1;
	}

	//경로간 상위, 하위, 겹치는 경우 예외처리
	for (int i = 0; i<info->exclude_path_cnt; i++) {
		size_t len1 = strlen(info->exclude_path[i]);
		size_t len2 = strlen(exclude);

		if (strncmp(exclude, info->exclude_path[i], len1) == 0) {
			if (exclude[len1] == '/' || exclude[len1] == '\0') {
				fprintf(stderr, "Error : Exclude path include Exclude path\n");
				return -1;
			}
		}

		if (strncmp(info->exclude_path[i], exclude, len2) == 0) {
			if (info->exclude_path[i][len2] == '/' || info->exclude_path[i][len2] == '\0') {
				fprintf(stderr, "Error : Exclude path include Exclude path\n");
				return -1;
			}
		}
	}

	return 0;
}
