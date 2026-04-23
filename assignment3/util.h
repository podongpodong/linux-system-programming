#include <dirent.h>
#include <stdint.h>

#ifndef UTIL_H
#define UTIL_H

typedef struct TreeNode {
    char name[256]; // 파일 또는 디렉토리 이름
    uint16_t i_mode; // ext2_inode의 i_mode 필드 값.
    uint32_t i_size; // ext2_inode의 i_size 필드 값.
    int depth; // 현재 노드의 트리에서의 깊이.
    // 자식 노드 중 첫 번째 자식을 가리키는 포인터입니다.
    struct TreeNode *firstChild;
    // 현재 디렉토리 내의 다음 항목을 가리킵니다.
    struct TreeNode *nextSibling;
} TreeNode;

// print 명령어에서 파일 데이터 블록 번호를 저장하기 위한 링크드 리스트 노드
typedef struct DataBlockNode {
    uint32_t block_num;
    struct DataBlockNode *next;
} DataBlockNode;

//Tree function
void tree(int argc, char *argv[]);

//Print function
void print_command(int argc, char **argv);

//Help function
void help_command(const char* cmd);

char *expand_home(const char* path);
int validate_path(const char* path);
int make_path(char* dst, const char* src1, const char* src2);

#endif
