#ifndef SHOW_H
#define SHOW_H

struct PathNode {
	int idx;
	char* path;
	struct PathNode* next;
};

extern struct PathNode* head;

//show() main
void show();

//문자열이 숫자로 구성된건지 확인
int isNumber(char* processIdx);

//디몬 목록을 읽어온다.
void read_daemon_list();

//config파일을 출력
void show_config(char* dir_path);

//log파일을 출력
void show_log(char* dir_path);

void addPathNode(char* dir_path, int line);
struct PathNode* findPathNode(int idx);

void freePathList();

#endif
