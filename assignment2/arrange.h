#ifndef ARRANGE_H
#define ARRANGE_H

#include<limits.h>
#include<time.h>
#include "parse_option.h"

#define bool int
#define true 1
#define false 0

typedef struct _node {
	char name[PATH_MAX];
	char path[PATH_MAX];
	char extension[50];
	time_t mtime;
	bool is_dir;
	bool is_checked;
	
	struct _node* next;
	struct _node* prev;
} Node;

typedef struct _list {
	Node* head;
	Node* tail;
	Node* begin;
	Node* end;
	Node dummy;
	unsigned size;
} List;

//파일을 탐색하는 함수
void traverse_dir(char* dir_path);
//arrange기능의 메인 함수
void arrange(struct OptionInfo* opInfo);
//동일한 이름의 파일은 config파일 정보에 따라서 삭제
void remove_same_name(struct OptionInfo* opInfo);
//디버깅용 함수
void print_list(List* list);
//파일을 정해진 디렉터리로 이동한다.
int move_file(const List* files_to_arrange, struct OptionInfo* opInfo);
//파일을 복사한다.
void copy_file(const char *src, const char *dst);
//복사한 파일을 로그에 기록한다.
void write_log(struct OptionInfo *opInfo, char *src_path, char *dst_path);
//-l이 적용된 경우 로그의 길이를 조절한다.
void adjust_log_file(struct OptionInfo *opInfo);

//파일 목록을 구성하는 링크드리스트 함수
List* list_init();	
void insert_front(List* list, Node* node);
void insert_back(List* list, Node* node);
Node* list_delete(List* list, Node* node);
void freeNode(List* list);


#endif
