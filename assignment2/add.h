#ifndef ADD_H
#define ADD_H

#include "parse_option.h"

//add함수의 시작점
void add();
//모니터링 가능한 경로인지 확인
int is_valid_monitoring(char *dir_path);
//초기 config파일 설정
void set_config(char* real_dir_path, char* new_dir_path);

//daemon 추가
void add_daemon(char* new_file_path);

//config파일에 OptionInfo구조체의 정보 입력
void write_config(char* new_file_path, struct OptionInfo *opInfo);
//config파일의 정보를 OptionInfo구조체로 불러온다.
struct OptionInfo* load_config(char* dir_path);
//daemon이 하는 작업 : arrange하여 파일 정리
void daemon_task(char *dir_path);

#endif
