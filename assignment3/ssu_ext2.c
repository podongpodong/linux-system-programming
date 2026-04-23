#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <errno.h>
#include <limits.h>

#include "ext2_struct.h"
#include "ssu_ext2.h"

static int fd;   
static unsigned int block_size;
static unsigned int inode_size; // sb.s_inode_size와 동일
static ext2_super_block sb;
static ext2_group_desc gd;


int load_ext2_image(char *path) {
    if ((fd = open(path, O_RDONLY)) < 0) {
		perror("open");
		return -1;
	}

    if (lseek(fd, 1024, SEEK_SET) < 0) {
		perror("lseek superblock");
        close(fd);
		return -1;
    }

    if (read(fd, &sb, sizeof(ext2_super_block)) != sizeof(ext2_super_block)) {
		perror("read superblock");
        close(fd); 
		return -1;
    }

	if (sb.s_magic != 0xEF53) {
		fprintf(stderr, "Invalid ext2 magic number. Not an ext2 file system.\n");
        close(fd);
		return -1;
    }

	block_size = 1024 << sb.s_log_block_size;
	inode_size = sb.s_inode_size;

 	off_t gd_offset = (sb.s_first_data_block + 1) * block_size;
    if (lseek(fd, gd_offset, SEEK_SET) < 0 || read(fd, &gd, sizeof(ext2_group_desc)) != sizeof(ext2_group_desc)) {
        perror("read group descriptor");
        close(fd);
		return -1;
    }
	return 0;
}

int ext2_close()
{
    return close(fd);
}


int get_ext2_fd() {
    return fd;
}

unsigned int get_block_size() {
    return block_size;
}

unsigned int get_inode_size() {
    return inode_size;
}

static int read_group_desc(uint32_t inum, ext2_group_desc *group_desc)
{
	uint32_t group_num = (inum - 1) / sb.s_inodes_per_group;
    off_t group_desc_table_start_block;

    group_desc_table_start_block = sb.s_first_data_block + 1;
    if (block_size == 1024 && sb.s_first_data_block == 1) {
        group_desc_table_start_block = 2;
    }
        
    off_t group_desc_offset = (group_desc_table_start_block * block_size) + (group_num * sizeof(ext2_group_desc));

    if (lseek(fd, group_desc_offset, SEEK_SET) < 0) {
        perror("read_inode: lseek for group_desc");
        return -1;
    }
    if (read(fd, group_desc, sizeof(ext2_group_desc)) != sizeof(ext2_group_desc)) {
        perror("read_inode: read for group_desc");
        return -1;
    }
	return 0;
}

// 아이노드 번호로 ext2_inode 구조체 데이터를 읽어옵니다.
int read_inode(uint32_t inum, ext2_inode *inode_buffer_target) {
    if (inum == 0) {
        return -1;
    }
    if (inum > sb.s_inodes_count) {
        return -1;
    }

    uint32_t inode_index_in_group = (inum - 1) % sb.s_inodes_per_group;
	ext2_group_desc group_desc;

	if (read_group_desc(inum, &group_desc) < 0) {
        return -1;
    }

    off_t inode_table_base_offset = (off_t)group_desc.bg_inode_table * block_size;
    off_t target_inode_offset = inode_table_base_offset + (inode_index_in_group * inode_size);

    unsigned char temp_inode_read_buffer[256]; // sb.s_inode_size가 256을 넘지 않는다고 가정

    if (lseek(fd, target_inode_offset, SEEK_SET) < 0) {
        perror("read_inode: lseek for inode data");
        return -1;
    }
    if (read(fd, temp_inode_read_buffer, inode_size) != inode_size) {
        perror("read_inode: read for inode data");
        return -1;
    }
    memcpy(inode_buffer_target, temp_inode_read_buffer, sizeof(ext2_inode));
    return 0;
}

static int get_next_path_token(const char* path, char* current_token, size_t tok_len, const char** next_path)
{
    const char *next_slash = strchr(path, '/');

    if (next_slash != NULL) {
        size_t token_len = next_slash - path;
        if (token_len == 0) { 
            return 2;
        }
        if (token_len >= tok_len) {
            fprintf(stderr, "Error: Path component is too long.\n");
            return 0;
        }
        strncpy(current_token, path, token_len);
        current_token[token_len] = '\0';
        *next_path = next_slash + 1;
    } else {
        if (strlen(path) >= tok_len) {
            fprintf(stderr, "Error: Path component is too long.\n");
            return 0;
        }
        strcpy(current_token, path);
		*next_path = NULL;
    }
    return 1;
}

// 경로를 재귀적으로 탐색하여 최종 아이노드 번호를 반환합니다.
static uint32_t get_inode_num(uint32_t current_dir_inode_num, const char *remaining_path) {
    if (remaining_path == NULL || *remaining_path == '\0') {
        return current_dir_inode_num;
    }

    ext2_inode current_dir_inode_data;
    if (read_inode(current_dir_inode_num, &current_dir_inode_data) != 0) {
        return 0;
    }
    if (!S_ISDIR(current_dir_inode_data.i_mode)) {
        return 0;
    }

    char current_token[256];
	const char *next_path = NULL;
	int e_code = get_next_path_token(remaining_path, current_token, sizeof(current_token), &next_path);
	if (e_code == 0)
		return 0;
	else if (e_code == 2)
		return get_inode_num(current_dir_inode_num, remaining_path + 1);	 

    char *data_block_buffer = (char*)malloc(block_size);
    if (!data_block_buffer) {
        perror("get_inode_num: malloc");
        return 0;
    }

    ext2_dir_entry_2 *dir_entry;
    unsigned int bytes_processed_in_block;
    uint32_t found_inode_num = 0;
    int entry_located = 0;

    for (int i = 0; i < 12 && current_dir_inode_data.i_block[i] != 0 && !entry_located; i++) {
        uint32_t data_block_num = current_dir_inode_data.i_block[i];
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
                if (dir_entry->name_len == strlen(current_token) &&
                    strncmp(dir_entry->name, current_token, dir_entry->name_len) == 0) {
                    found_inode_num = dir_entry->inode;
                    entry_located = 1;
                    break;
                }
            }
            bytes_processed_in_block += dir_entry->rec_len;
            if (bytes_processed_in_block >= block_size) break;
            dir_entry = (ext2_dir_entry_2 *)((char *)data_block_buffer + bytes_processed_in_block);
        }
    }

    free(data_block_buffer);

    if (entry_located) {
        return get_inode_num(found_inode_num, next_path);
    } else {
        return 0;
    }
}

uint32_t start_get_inode(const char* path) 
{
	if (!path || path[0] == '\0') {
        return 0;
    }

    uint32_t start_inode;
    const char *path_to_resolve;

    if (path[0] == '/') { 
        start_inode = 2; // EXT2_ROOT_INO
        path_to_resolve = path + 1;
        if (*path_to_resolve == '\0') return start_inode;
    } else if (strcmp(path, ".") == 0) {
         return 2; // EXT2_ROOT_INO
    } else { 
        start_inode = 2; // EXT2_ROOT_INO
        path_to_resolve = path;
    }

    if (path_to_resolve == NULL || *path_to_resolve == '\0') {
        return start_inode;
    }
    
    return get_inode_num(start_inode, path_to_resolve);
}
