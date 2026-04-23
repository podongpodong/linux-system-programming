#ifndef TREE_UTIL_H
#define TREE_UTIL_H

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

TreeNode* build_tree(const char *path, int depth);
void print_node(TreeNode *node);
void print_tree(TreeNode *node, int level);
void free_tree_node(TreeNode *node);
void set_tree_info(int r, int s, int p);
#endif