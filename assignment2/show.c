#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<ctype.h>

#include "ssu_cleanupd.h"
#include "show.h"

struct PathNode* head;

void show() {
	if (parse_count > 1) {
		fprintf(stderr, "Too many arguement\n");
		return;
	}

	printf("Current working daemon process list\n\n");

	fd_daemon_list = fopen(daemon_list_path, "r");
	//current_daemon_list의 목록을 연결리스트로 구성
	read_daemon_list();
	fclose(fd_daemon_list);
	
	while(1) {
		struct PathNode* temp = head;

		printf("%d. exit\n", 0);
		while(temp != NULL) {
			printf("%d. %s\n", temp->idx, temp->path);
			temp = temp->next;
		}
		printf("\n");

		char processIdx[5000];
		printf(" Select one to see process info : ");
		fgets(processIdx, sizeof(processIdx), stdin);
		processIdx[strcspn(processIdx, "\n")] = 0;

		//주어진 번호이외의 입력이 주어진 경우 예외처리
		long idx;
		if (strlen(processIdx) == 0 || isNumber(processIdx) < 0) {
			printf("Please check your input is vaild\n\n");
		}
		else {
			idx = atol(processIdx);
			if(idx == 0) break;
			struct PathNode* target = findPathNode(idx);
			if(target == NULL) {
				printf("Please check your input is vaild\n\n");
				continue;
			}

			printf("%ld %s\n", idx, target->path);
			show_config(target->path);
			show_log(target->path);
			break;
		}
	}	

	freePathList();
}

int isNumber(char* processIdx) {
	for(int i = 0; i<strlen(processIdx); i++) {
		if(!isdigit(processIdx[i]))
				return -1;
	}
	return 0;
}

void read_daemon_list() {
	int lineNum = 1;
	char lineBuf[5000];
	while(fgets(lineBuf, sizeof(lineBuf), fd_daemon_list) != NULL) {
		addPathNode(lineBuf, lineNum);
		lineNum+=1;
	}
}

void show_config(char* dir_path) {
	char file_path[5000];
	snprintf(file_path, sizeof(file_path), "%s/ssu_cleanupd.config", dir_path);

	FILE* fp = fopen(file_path, "r");
	if(fp == NULL) fprintf(stderr, "fopen error for %s\n", file_path);
	char lineBuf[5000];

	printf("\n1. config detail\n");
	while(fgets(lineBuf, sizeof(lineBuf), fp) != NULL) {
		printf("%s", lineBuf);
	}
	printf("\n");

	fclose(fp);
}

void show_log(char* dir_path) {
	char file_path[5000];
	snprintf(file_path, sizeof(file_path), "%s/ssu_cleanupd.log", dir_path);

	FILE* fp = fopen(file_path, "r");
	if(fp == NULL) fprintf(stderr, "fopen error for %s\n", file_path);
	char lineBuf[5000];
	printf("2. log detail\n");
	while(fgets(lineBuf, sizeof(lineBuf), fp) != NULL) {
		printf("%s", lineBuf);
	}
	printf("\n");

	fclose(fp);
}

void addPathNode(char* dir_path, int line) {
	struct PathNode* newNode = (struct PathNode*)calloc(1, sizeof(struct PathNode));
	newNode->idx = line;
	newNode->path = strdup(dir_path);
	newNode->path[strcspn(newNode->path, "\n")] = 0;
	newNode->next = NULL;

	if(head == NULL)
		head = newNode;
	else {
		struct PathNode* temp = head;
		while(temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = newNode;
	}
}

struct PathNode* findPathNode(int idx) {
	struct PathNode* temp = head;
	while(temp != NULL) {
		if(temp->idx == idx)
			return temp;
		temp = temp->next;
	}
	return NULL;
}

void freePathList() {
	struct PathNode* temp = head;
	while(temp != NULL) {
		struct PathNode* tempNext = temp->next;

		free(temp->path);
		free(temp);

		temp = tempNext;
	}
	head = NULL;
}
