#ifndef PARSE_OPTION_H
#define PARSE_OPTION_H
#include<limits.h>
#include<sys/types.h>

struct OptionInfo {
	char monitoring_path[PATH_MAX];
	pid_t pid;
	time_t start_time;
	char output_path[PATH_MAX]; // -d default : <dir_path>_arranged
	int time_interval; // -i
	int max_log_lines; // -l default : none
	char *exclude_path[4096];  // -x default : nonu
	int exclude_path_cnt; 
	char *extensions[4096]; //-e default : all
	int extensions_cnt;
	int mode_number; // -m default : 1
};

extern int flag_d;
extern int flag_i;
extern int flag_l;
extern int flag_x;
extern int flag_e;
extern int flag_m;

extern struct OptionInfo option_init;

extern const int DEFAULT_TIME;
extern const int DEFAULT_MAX_LOG_LINES;
extern const int DEFAULT_MODE;

int parse_option();
//경로가 유효한지 확인
int is_valid_dir(char *abs_path, char *dir_path);
//-d 옵션의 인자가 유효한지
int is_valid_output(char *dir_path, char *output);
//-e 옵션의 인자가 유효한지 확인
int is_valid_exclude(struct OptionInfo *info, char *exclude);

#endif
