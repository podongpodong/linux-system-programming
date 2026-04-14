#ifndef HASHTABLE_H 
#define HASHTABLE_H

//save file information
typedef struct PathNode{
	int idx;
    char *path; //<dir_path> start arranged dir name
	char *abs_path; //path file's absolute path
    struct PathNode* next;
}PathNode;

//save filename, if fname same make linked list
typedef struct HashNode {
	int cnt;
	char* filename;
	PathNode* path_list; // save duplicated file path and absolutepath
	struct HashNode *next;
}HashNode;

typedef struct HashTable {
	HashNode *buckets;
}HashTable;

extern HashTable *ht;

PathNode* get_file(PathNode* cur_path, int idx);

//insert file in hashtable
void insertFile(char* filename, char*filepath, char* abs_path);

//make HashNode
HashNode* makeHashNode(char* fname);
//make PathNode
PathNode* makePathNode(char* fpath, char* abs_path);

void print_hashtable(void);

//free HashTable
void freeHashTable(void);

#endif
