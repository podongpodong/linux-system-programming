// tree.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <sys/stat.h>

#include "ext2_struct.h"
#include "ssu_ext2.h"
#include "tree_util.h"

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

// 명령행 인자를 파싱하여 옵션 플래그를 설정하고 경로를 가져옵니다.
static int parse_option(int argc, char *argv[], char **target_path_out) {
    int opt;
    *target_path_out = NULL;

    if (argc < 2) {
        fprintf(stderr, "Error: Path argument is missing for tree.\n");
        help_usage_tree();
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
                help_usage_tree();
                return -1;
        }
    }

    // 옵션 외에 남는 인자가 있으면 오류
    if (optind < argc) {
        fprintf(stderr, "Error: Too many arguments for tree.\n");
        help_usage_tree();
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

    uint32_t inode_num_check = get_inode_num(path);
    if (inode_num_check == 0) {
        // 올바르지 않은 경로(존재하지 않는 디렉토리)
        fprintf(stderr, "Error: Cannot access '%s': No such file or directory\n", path);
        help_usage_tree(); // help.c의 함수 호출
        return;
    }

    set_tree_info(tinfo.r_flag, tinfo.s_flag, tinfo.p_flag);
    TreeNode *root_display_node = build_tree(path, 0);
    
    if (!root_display_node) {
        return;
    }

    memset(parent_info, 0, sizeof(parent_info)); 

    print_node(root_display_node);

    TreeNode *child_ptr = root_display_node->firstChild;
    while (child_ptr) {
        if (1 < MAX_TREE_DEPTH) { 
            parent_info[1] = (child_ptr->nextSibling == NULL);
        }
        print_tree(child_ptr, 1);
        child_ptr = child_ptr->nextSibling;
    }

    printf("\n%d directories, %d files\n", tinfo.dir_count, tinfo.file_count);

    free_tree_node(root_display_node);
}