#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

#include "ext2_struct.h"
#include "ssu_ext2.h"
#include "tree_util.h"

#define MAX_TREE_DEPTH 2048
// parent_info[level]이 1(true)이면, 해당 level의 노드가 형제 중 마지막임을 나타냄

struct tree_option{
    int r_flag;
    int s_flag;
    int p_flag;
    int parent_info[MAX_TREE_DEPTH];
};

static struct tree_option option;

void set_tree_info(int r, int s, int p) {
    option.r_flag = r;
    option.s_flag = s;
    option.p_flag = p;
}

// i_mode 값을 기반으로 파일 권한 문자열을 생성합니다.
static void format_permissions(uint16_t mode, char *perm_str) {
    if (!perm_str) return;
    perm_str[0] = S_ISDIR(mode) ? 'd' : (S_ISREG(mode) ? '-' : (S_ISLNK(mode) ? 'l' : '?'));
    if (perm_str[0] == '?') {
         if (S_ISCHR(mode)) perm_str[0] = 'c';
         else if (S_ISBLK(mode)) perm_str[0] = 'b';
         else if (S_ISFIFO(mode)) perm_str[0] = 'p';
         else if (S_ISSOCK(mode)) perm_str[0] = 's';
    }
    perm_str[1] = (mode & S_IRUSR) ? 'r' : '-';
    perm_str[2] = (mode & S_IWUSR) ? 'w' : '-';
    perm_str[3] = (mode & S_IXUSR) ? 'x' : '-';
    perm_str[4] = (mode & S_IRGRP) ? 'r' : '-';
    perm_str[5] = (mode & S_IWGRP) ? 'w' : '-';
    perm_str[6] = (mode & S_IXGRP) ? 'x' : '-';
    perm_str[7] = (mode & S_IROTH) ? 'r' : '-';
    perm_str[8] = (mode & S_IWOTH) ? 'w' : '-';
    perm_str[9] = (mode & S_IXOTH) ? 'x' : '-';
    perm_str[10] = '\0';
}

// 새로운 TreeNode를 생성하고 초기화합니다.
static TreeNode* create_node(const char *name, uint16_t i_mode, uint32_t i_size, int depth) {
    TreeNode *new_node = (TreeNode*)malloc(sizeof(TreeNode));
    if (!new_node) {
        perror("create_node: malloc TreeNode");
        return NULL;
    }
    if (name) {
        strncpy(new_node->name, name, sizeof(new_node->name) - 1);
        new_node->name[sizeof(new_node->name) - 1] = '\0';
    } else {
        new_node->name[0] = '\0';
    }
    new_node->i_mode = i_mode;
    new_node->i_size = i_size;
    new_node->depth = depth; 
    new_node->firstChild = NULL;
    new_node->nextSibling = NULL;
    return new_node;
}

// TreeNode 메모리 해제
void free_tree_node(TreeNode *node) {
    if (node == NULL) return;
    free_tree_node(node->firstChild);
    free_tree_node(node->nextSibling);
    free(node);
}

// 디렉토리 아이노드의 엔트리들을 읽어 TreeNode 자식 노드들을 생성하고 추가합니다.
static void build_dir_nodes(uint32_t dir_inode_num, TreeNode *parent_node_for_children) {
    if (!parent_node_for_children) return;
    
    ext2_inode dir_inode_data;
    if (read_inode(dir_inode_num, &dir_inode_data) != 0) return;
    if (!S_ISDIR(dir_inode_data.i_mode)) return;

    char *data_block_buffer = (char*)malloc(block_size);
    if (!data_block_buffer) {
        perror("build_dir_nodes: malloc data_block_buffer");
        return;
    }

    ext2_dir_entry_2 *dir_entry;
    unsigned int bytes_processed_in_block;

    for (int i = 0; i < 12; i++) { 
        uint32_t data_block_num = dir_inode_data.i_block[i];
        if (data_block_num == 0) continue;

        if (lseek(fd, (off_t)data_block_num * block_size, SEEK_SET) < 0) {
            continue;
        }
        if (read(fd, data_block_buffer, block_size) != block_size) {
            continue;
        }

        bytes_processed_in_block = 0;
        dir_entry = (ext2_dir_entry_2 *)data_block_buffer;

        while (bytes_processed_in_block < block_size) {
            if (dir_entry->rec_len == 0 || dir_entry->rec_len < 8 || (bytes_processed_in_block + dir_entry->rec_len > block_size)) {
                 break;
            }

            if (dir_entry->inode != 0 && dir_entry->name_len > 0) {
                if (dir_entry->name_len < sizeof(dir_entry->name)) {
                    char entry_name_buffer[256]; 
                    strncpy(entry_name_buffer, dir_entry->name, dir_entry->name_len);
                    entry_name_buffer[dir_entry->name_len] = '\0';

                    if (strcmp(entry_name_buffer, ".") != 0 &&
                        strcmp(entry_name_buffer, "..") != 0 &&
                        strcmp(entry_name_buffer, "lost+found") != 0) {

                        ext2_inode child_inode_data;
                        if (read_inode(dir_entry->inode, &child_inode_data) == 0) {
                            TreeNode *new_child_node = create_node(
                                entry_name_buffer,
                                child_inode_data.i_mode,
                                child_inode_data.i_size,
                                parent_node_for_children->depth + 1 
                            );

                            if (new_child_node) {
                                add_node(parent_node_for_children, new_child_node);
                                if (S_ISDIR(child_inode_data.i_mode)) {
                                    current_dir_count++;
                                    if (option.r_flag) { 
                                        build_dir_nodes(dir_entry->inode, new_child_node);
                                    }
                                } else {
                                    current_file_count++;
                                }
                            }
                        }
                    }
                }
            }
            bytes_processed_in_block += dir_entry->rec_len;
            if (bytes_processed_in_block >= block_size) break;
            dir_entry = (ext2_dir_entry_2 *)((char *)data_block_buffer + bytes_processed_in_block);
        }
    }
    free(data_block_buffer);
}

// 지정된 경로로부터 시작하여 파일/디렉토리 트리를 구축합니다.
TreeNode* build_tree(const char *path, int depth) {
    if (!path) return NULL;

    uint32_t start_inode_num = start_get_inode(path);
    if (start_inode_num == 0) { 
		return NULL; 
	}

    ext2_inode path_inode_data;
    if (read_inode(start_inode_num, &path_inode_data) != 0) {
        return NULL; 
    }

    if (!S_ISDIR(path_inode_data.i_mode)) {
        // 첫 번째 인자가 디렉토리가 아닐 경우 에러 메세지 출력
        fprintf(stderr, "Error: '%s' is not a directory\n", path);
        return NULL;
    }

    char display_root_name[256];
    const char *last_slash = strrchr(path, '/');

    if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0) { 
        strncpy(display_root_name, path, sizeof(display_root_name) -1);
    } else if (last_slash && *(last_slash + 1) != '\0') {
        strncpy(display_root_name, last_slash + 1, sizeof(display_root_name) - 1);
    } else if (last_slash == NULL && strlen(path) > 0) {
         strncpy(display_root_name, path, sizeof(display_root_name) -1);
    } else { 
        strncpy(display_root_name, path, sizeof(display_root_name) -1);
    }
    display_root_name[sizeof(display_root_name)-1] = '\0';

    TreeNode *root_node = create_node(display_root_name, path_inode_data.i_mode, path_inode_data.i_size, depth);
    if (!root_node) return NULL;

    current_dir_count++; 
    build_dir_nodes(start_inode_num, root_node);
    return root_node;
}

// 부모 TreeNode에 자식 TreeNode를 추가합니다.
static void add_node(TreeNode *parent, TreeNode *child_to_add) {
    if (!parent || !child_to_add) return;
    child_to_add->nextSibling = NULL; 

    if (parent->firstChild == NULL) {
        parent->firstChild = child_to_add;
    } else {
        TreeNode *current_sibling = parent->firstChild;
        while (current_sibling->nextSibling != NULL) {
            current_sibling = current_sibling->nextSibling;
        }
        current_sibling->nextSibling = child_to_add;
    }
}

// 트리 출력 시 사용되는 가지 모양을 출력합니다.
static void print_tree_branch(int level) {
    for (int i = 1; i < level; i++) {
        printf("%s", option.parent_info[i] ? "    " : "│   ");
    }
    if (level > 0) {
        printf("%s", option.parent_info[level] ? "└── " : "├── ");
    }
}

// TreeNode의 정보를 포맷에 맞춰 출력합니다.
void print_node(TreeNode *node) {
    if (!node) return;

    char perm_str[11]; 
    if (option.p_flag) {
        format_permissions(node->i_mode, perm_str);
    }

    if (option.s_flag && option.p_flag) {
        printf("[%s %u] %s\n", perm_str, node->i_size, node->name);
    } else if (option.s_flag) {
        printf("[%u] %s\n", node->i_size, node->name);
    } else if (option.p_flag) {
        printf("[%s] %s\n", perm_str, node->name);
    } else {
        printf("%s\n", node->name);
    }
}

// 구축된 TreeNode 트리를 재귀적으로 화면에 출력합니다.
void print_tree(TreeNode *node, int level) {
    if (!node) return;
    if (level >= MAX_TREE_DEPTH) {
        fprintf(stderr, "Warning: Tree depth %d exceeds MAX_TREE_DEPTH (%d). Truncating display.\n", level, MAX_TREE_DEPTH);
        return;
    }

    print_tree_branch(level);
    print_node(node);

    TreeNode *child = node->firstChild;
    while (child) {
        if (level + 1 < MAX_TREE_DEPTH) {
            option.parent_info[level + 1] = (child->nextSibling == NULL);
        }
        print_tree(child, level + 1);
        child = child->nextSibling;
    }
}
