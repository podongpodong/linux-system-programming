// print.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <ctype.h>

#include "util.h"
#include "ext2_struct.h"
#include "ssu_ext2.h"

// --- 함수 프로토타입 ---
static void print_file_content_ll(ext2_inode *file_inode, int max_lines);
static int read_data_block_content(uint32_t block_num, char *buffer, uint32_t *bytes_read_from_block);
static void collect_data_block_numbers(uint32_t block_ptr_block_num, int level, DataBlockNode **list_head, DataBlockNode **list_tail);
static void free_block_list(DataBlockNode *head);
static void add_block_to_list(DataBlockNode **head, DataBlockNode **tail, uint32_t block_num);

// print 명령어 메인 함수
void print_command(int argc, char **argv) {
    char *path = NULL;
    int line_number = -1; // -1은 전체 출력을 의미

    if (argc < 2) {
        help_command("print"); // Usage 출력
        return;
    }

    // 옵션 및 경로 파싱
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                for (int k = 0; argv[i+1][k] != '\0'; k++) {
                    if (!isdigit(argv[i+1][k])) {
                        fprintf(stderr, "print: option requires a numeric argument for -n\n\n");
                        help_command("print"); // Usage 출력
                        return;
                    }
                }
                line_number = atoi(argv[i+1]);
                if (line_number <= 0) {
                    fprintf(stderr, "print: line number for -n must be positive\n\n");
                    help_command("print"); // Usage 출력
                    return;
                }
                i++; // 숫자 인자까지 처리
            } else {
                fprintf(stderr, "print: option requires an argument -- 'n'\n\n");
                return; // 명세서에 따라 Usage 없이 메시지만 출력
            }
        } else if (argv[i][0] == '-') { // 알 수 없는 옵션
            fprintf(stderr, "print: invalid option -- '%c'\n", argv[i][1]);
            help_command("print"); // Usage 출력
            return;
        } else { // 옵션이 아닌 첫 번째 인자를 경로로 간주
            if (path == NULL) {
                path = argv[i];
            } else { // 경로 인자 뒤에 또 다른 비옵션 인자가 온 경우
                fprintf(stderr, "Error: Too many arguments for print.\n");
                help_command("print"); // Usage 출력
                return;
            }
        }
    }

    if (path == NULL) { // 경로가 지정되지 않은 경우
        fprintf(stderr, "Error: Path argument is missing for print.\n");
        help_command("print"); // Usage 출력
        return;
    }

    uint32_t inode_num = start_get_inode(path); // 공용 함수 사용
    if (inode_num == 0) {
        help_command("print"); // Usage 추가 출력
        return;
    }

    ext2_inode file_inode;
    if (read_inode(inode_num, &file_inode) != 0) {
        fprintf(stderr, "Error: Failed to read inode for %s (inode %u)\n", path, inode_num);
        return;
    }

    if (!S_ISREG(file_inode.i_mode)) {
        fprintf(stderr, "Error: '%s' is not a file\n\n", path); // 에러 메시지만 출
        return;
    }

    print_file_content_ll(&file_inode, line_number);
	printf("\n");
}

// 데이터 블록 리스트에 새 블록 번호 추가
static void add_block_to_list(DataBlockNode **head, DataBlockNode **tail, uint32_t block_num) {
    DataBlockNode *new_node = (DataBlockNode *)malloc(sizeof(DataBlockNode));
    if (!new_node) {
        perror("add_block_to_list: malloc failed");
        return;
    }
    new_node->block_num = block_num;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        *tail = new_node;
    } else {
        (*tail)->next = new_node;
        *tail = new_node;
    }
}

// 데이터 블록 리스트 메모리 해제
static void free_block_list(DataBlockNode *head) {
    DataBlockNode *current = head;
    DataBlockNode *next_node;
    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}

// 재귀적으로 간접 블록을 탐색하여 데이터 블록 번호를 링크드 리스트에 수집
static void collect_data_block_numbers(uint32_t block_ptr_block_num, int level,
                                       DataBlockNode **list_head, DataBlockNode **list_tail) {
    if (block_ptr_block_num == 0) return;

    if (level == 0) { // 실제 데이터 블록 번호
        add_block_to_list(list_head, list_tail, block_ptr_block_num);
        return;
    }
    
    int fd = get_ext2_fd();
    unsigned int block_size = get_block_size();
    uint32_t *indirect_block_pointers = (uint32_t*)malloc(block_size);
    if (!indirect_block_pointers) {
        perror("collect_data_block_numbers: malloc for indirect_block_pointers failed");
        return;
    }

    if (lseek(fd, (off_t)block_ptr_block_num * block_size, SEEK_SET) < 0) {
        perror("collect_data_block_numbers: lseek failed for indirect block");
        free(indirect_block_pointers);
        return;
    }

    ssize_t bytes_read = read(fd, indirect_block_pointers, block_size);

    if (bytes_read < 0) { // 읽기 오류
        perror("collect_data_block_numbers: read failed for indirect pointers");
        free(indirect_block_pointers);
        return;
    }
    // 간접 블록은 항상 block_size 만큼 채워져 있거나 0으로 패딩됨
    if ((uint32_t)bytes_read != block_size) {
        fprintf(stderr, "Warning: Incomplete read for indirect block %u (read %zd, expected %u bytes)\n",
                block_ptr_block_num, bytes_read, block_size);
    }

    int num_pointers_to_process = bytes_read / sizeof(uint32_t); // 실제 읽은 바이트 기준 처리할 포인터 수

    for (int i = 0; i < num_pointers_to_process; i++) {
        uint32_t next_block_num = indirect_block_pointers[i];
        if (next_block_num == 0) { // 0은 유효한 다음 블록 포인터가 아님
            continue;
        }
        collect_data_block_numbers(next_block_num, level - 1, list_head, list_tail);
    }

    free(indirect_block_pointers);
}

// 실제 데이터 블록의 내용을 읽어오는 함수
static int read_data_block_content(uint32_t block_num, char *buffer, uint32_t *bytes_actually_read) {

    int fd = get_ext2_fd();
    unsigned int block_size = get_block_size();
    if (lseek(fd, (off_t)block_num * block_size, SEEK_SET) < 0) {
        perror("read_data_block_content: lseek failed");
        *bytes_actually_read = 0;
        return -1;
    }

    ssize_t ret = read(fd, buffer, block_size); // block_size만큼 읽기를 시도
    if (ret < 0) {
        perror("read_data_block_content: read failed");
        *bytes_actually_read = 0;
        return -1;
    }
    *bytes_actually_read = (uint32_t)ret; // 실제로 읽은 바이트 수 저장
    return 0;
}

// 파일 아이노드를 받아 내용을 출력
static void print_file_content_ll(ext2_inode *file_inode, int max_lines) {
    DataBlockNode *block_list_head = NULL;
    DataBlockNode *block_list_tail = NULL;

    // 직접 블록 번호 수집 (i_block[0] ~ i_block[11])
    for (int i = 0; i < 12; i++) {
        if (file_inode->i_block[i] == 0) continue;
        collect_data_block_numbers(file_inode->i_block[i], 0, &block_list_head, &block_list_tail);
    }

    // 단일 간접 블록 (i_block[12])
    if (file_inode->i_block[12] != 0) {
        collect_data_block_numbers(file_inode->i_block[12], 1, &block_list_head, &block_list_tail);
    }

    // 이중 간접 블록 (i_block[13])
    if (file_inode->i_block[13] != 0) {
        collect_data_block_numbers(file_inode->i_block[13], 2, &block_list_head, &block_list_tail);
    }

    // 삼중 간접 블록 (i_block[14])
    if (file_inode->i_block[14] != 0) {
        collect_data_block_numbers(file_inode->i_block[14], 3, &block_list_head, &block_list_tail);
    }

    unsigned int block_size = get_block_size();
    char *data_block_buffer = (char*)malloc(block_size);
    if (!data_block_buffer) {
        perror("print_file_content_ll: malloc for data_block_buffer failed");
        free_block_list(block_list_head);
        return;
    }

    uint32_t total_bytes_to_print = file_inode->i_size;
    int lines_printed = 0;
    int end_printing = 0;
    DataBlockNode *current_block_node = block_list_head;

    while (current_block_node != NULL && total_bytes_to_print > 0 && !end_printing) {
        uint32_t bytes_actually_read_from_block;
        if (read_data_block_content(current_block_node->block_num, data_block_buffer, &bytes_actually_read_from_block) != 0) {
            break; // 블록 읽기 실패 시 중단
        }

        if (bytes_actually_read_from_block == 0) { // 블록에서 읽은 내용이 없는 경우
            current_block_node = current_block_node->next;
            continue;
        }

        // 현재 블록에서 실제 출력할 바이트 수
        uint32_t bytes_to_process_in_this_block = (total_bytes_to_print < bytes_actually_read_from_block) ? total_bytes_to_print : bytes_actually_read_from_block;

        for (uint32_t j = 0; j < bytes_to_process_in_this_block && !end_printing; j++) {
            putchar(data_block_buffer[j]);
            if (max_lines > 0 && data_block_buffer[j] == '\n') {
                lines_printed++;
                if (lines_printed >= max_lines) {
                    end_printing = 1;
                }
            }
        }
        total_bytes_to_print -= bytes_to_process_in_this_block;
        current_block_node = current_block_node->next;
    }

    free(data_block_buffer);
    free_block_list(block_list_head);
}
