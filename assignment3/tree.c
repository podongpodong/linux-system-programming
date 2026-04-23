// tree.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <sys/stat.h>

#include "ext2_struct.h"
#include "util.h"
#include "ssu_ext2.h"

#define DIR_TOKEN 1
#define MAX_TREE_DEPTH 2048

struct tree_info {
    int r_flag;
    int s_flag;
    int p_flag;
    int dir_count;
    int file_count;
};

static struct tree_info tinfo;
static int parent_info[MAX_TREE_DEPTH];

// tree.c 내부 static 함수
static int parse_option(int argc, char *argv[], char **target_path_out);
static void init_tree_info(void);

// tree_util.c 내부 static 함수
static void build_dir_nodes(uint32_t dir_inode_num, TreeNode *parent_node_for_children);
static void add_node(TreeNode *parent, TreeNode *child_to_add);
static void format_permissions(uint16_t mode, char *perm_str);
static TreeNode* create_node(const char *name, uint16_t i_mode, uint32_t i_size, int depth);
static void print_tree_branch(int level);

// 외부 공개 함수
void tree(int argc, char *argv[]);
TreeNode* build_tree(const char *path, int depth);
void free_tree_node(TreeNode *node);
void print_node(TreeNode *node);
void print_tree(TreeNode *node, int level);

// 명령행 인자를 파싱하여 옵션 플래그를 설정하고 경로를 가져옵니다.
static int parse_option(int argc, char *argv[], char **target_path_out) {
    int opt;
    *target_path_out = NULL;

    if (argc < 2) {
        fprintf(stderr, "Error: Path argument is missing for tree.\n");
        help_command("tree");
        return -1;
    }

    *target_path_out = argv[DIR_TOKEN];

    // getopt 초기화
    optind = 2;
    opterr = 0;

    while ((opt = getopt(argc, argv, "rsp")) != -1) {
        switch (opt) {
            case 'r':
                tinfo.r_flag = 1;
                break;
            case 's':
                tinfo.s_flag = 1;
                break;
            case 'p':
                tinfo.p_flag = 1;
                break;
            case '?':
                fprintf(stderr, "tree: invalid option -- '%c'\n", optopt);
                help_command("tree");
                return -1;
        }
    }

    // 옵션 외에 남는 인자가 있으면 오류
    if (optind < argc) {
        fprintf(stderr, "Error: Too many arguments for tree.\n");
        help_command("tree");
        return -1;
    }

    return 0;
}

static void init_tree_info()
{
    tinfo.r_flag = 0;
    tinfo.s_flag = 0;
    tinfo.p_flag = 0;
    tinfo.dir_count = 0;
    tinfo.file_count = 0;
}

// tree 명령어의 메인 로직을 수행합니다.
void tree(int argc, char *argv[]) {
    char *path = NULL;

    init_tree_info();
    if (parse_option(argc, argv, &path) != 0) {
        return; // parse_option에서 오류 메시지 및 Usage 출력
    }

    uint32_t inode_num_check = start_get_inode(path);
    if (inode_num_check == 0) {
        // 올바르지 않은 경로(존재하지 않는 디렉토리)
        fprintf(stderr, "Error: Cannot access '%s': No such file or directory\n", path);
        help_command("tree"); // help.c의 함수 호출
        return;
    }

    TreeNode *root_node = build_tree(path, 0);
    
    if (!root_node) {
        return;
    }

    print_tree(root_node, 0);

    printf("\n%d directories, %d files\n", tinfo.dir_count, tinfo.file_count);

    free_tree_node(root_node);
}

static void build_dir_nodes(uint32_t dir_inode_num, TreeNode *parent_node_for_children) {
    if (!parent_node_for_children) return;
    
    int fd = get_ext2_fd();
    unsigned int block_size = get_block_size();
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
                                    tinfo.dir_count++;
                                    if (tinfo.r_flag) { 
                                        build_dir_nodes(dir_entry->inode, new_child_node);
                                    }
                                } else {
                                    tinfo.file_count++;
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

    tinfo.dir_count++; 
    build_dir_nodes(start_inode_num, root_node);
    return root_node;
}

// TreeNode 메모리 해제
void free_tree_node(TreeNode *node) {
    if (node == NULL) return;
    free_tree_node(node->firstChild);
    free_tree_node(node->nextSibling);
    free(node);
}

// 트리 출력 시 사용되는 가지 모양을 출력합니다.
static void print_tree_branch(int level) {
    for (int i = 1; i < level; i++) {
        printf("%s", parent_info[i] ? "    " : "│   ");
    }
    if (level > 0) {
        printf("%s", parent_info[level] ? "└── " : "├── ");
    }
}

// TreeNode의 정보를 포맷에 맞춰 출력합니다.
void print_node(TreeNode *node) {
    if (!node) return;

    char perm_str[11]; 
    if (tinfo.p_flag) {
        format_permissions(node->i_mode, perm_str);
    }

    if (tinfo.s_flag && tinfo.p_flag) {
        printf("[%s %u] %s\n", perm_str, node->i_size, node->name);
    } else if (tinfo.s_flag) {
        printf("[%u] %s\n", node->i_size, node->name);
    } else if (tinfo.p_flag) {
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
            parent_info[level + 1] = (child->nextSibling == NULL);
        }
        print_tree(child, level + 1);
        child = child->nextSibling;
    }
}
