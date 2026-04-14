#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "hashtable.h"

HashTable *ht = NULL;

//Make newHashNode
HashNode* makeHashNode(char* fname) {
		HashNode* newHashNode = (HashNode*)malloc(sizeof(HashNode));
		newHashNode->filename = strdup(fname);
		newHashNode->next = NULL;
		return newHashNode;
}

//Make newPathNode
PathNode* makePathNode(char* fpath, char* abs_path) {
		PathNode* newPathNode = (PathNode*)malloc(sizeof(PathNode));
		newPathNode->path = strdup(fpath);
		newPathNode->abs_path = strdup(abs_path);
		newPathNode->next = NULL;
		return newPathNode;
}

void insertFile(char* fname, char* fpath, char* abs_path) {
	//if HashTable is empty
	if(ht->buckets == NULL) {
		HashNode* newHashNode = makeHashNode(fname);
		PathNode* newPathNode = makePathNode(fpath, abs_path);
		newPathNode->idx = 1;

		newHashNode->path_list = newPathNode;
		newHashNode->cnt = 1;
		ht->buckets = newHashNode;
		return ;
	}

	HashNode* cur = ht->buckets;

	//if HashTable doesn't empty
	while(cur != NULL) {
		if(strcmp(cur->filename, fname) == 0)
			break;
		cur=cur->next;
	}

	//if fname doesn't duplicate
	//add new HashNode
	if(cur == NULL) {
		HashNode* newHashNode = makeHashNode(fname);
		PathNode* newPathNode = makePathNode(fpath, abs_path);
		newPathNode->idx = 1;

		newHashNode->path_list = newPathNode;
		newHashNode->cnt = 1;
		newHashNode->next=ht->buckets;
		ht->buckets=newHashNode;
	}
	//if fname duplicated, add newPathNode in path_list
	else {
		cur->cnt+=1;
		PathNode* cur_path = cur->path_list;
		while(cur_path->next != NULL)
			cur_path=cur_path->next;
		
		PathNode* newPathNode = (PathNode*)malloc(sizeof(PathNode));
		newPathNode->idx = cur_path->idx+1;
		newPathNode->path = strdup(fpath);
		newPathNode->abs_path = strdup(abs_path);
		newPathNode->next = NULL;
		cur_path->next = newPathNode;
	}
		
}

void print_hashtable(void) {
	HashNode* cur = ht->buckets;
	while(cur != NULL) {
		PathNode* cur_path = cur->path_list;
		printf("%s-%d\n", cur->filename, cur->cnt);
		while(cur_path != NULL) {
			printf("%d %s\n", cur_path->idx, cur_path->path);
			cur_path = cur_path->next;
		}
		cur=cur->next;
	}
}

void freeHashTable(void) {
	//free HashTable
	HashNode *cur = ht->buckets;
	while(cur != NULL) {
		HashNode *nextHashNode = cur->next;
		//free PathNode LinkedList
		PathNode *curPath = cur->path_list;
		while(curPath != NULL) {
			PathNode *nextPath = curPath->next;
			free(curPath->path);
			free(curPath->abs_path);
			free(curPath);
			curPath = nextPath;
		}
		free(cur->filename);
		free(cur);
		cur = nextHashNode;
	}
	free(ht);
}

//return file, cur->idx == idx
PathNode* get_file(PathNode* cur_path, int idx) {
	PathNode* cur = cur_path;
	while(cur != NULL) {
		//if idx exist in LinkedList
		if(cur->idx == idx) return cur;
		cur=cur->next;
	}
	//if idx no exist in LinkedList
	return NULL;
}

