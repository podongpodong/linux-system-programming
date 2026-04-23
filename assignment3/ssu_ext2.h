#ifndef SSU_EXT2_H
#define SSU_EXT2_H
#include <stdint.h>

#include "ext2_struct.h"

int load_ext2_image(char *path);
int ext2_close(void);
int get_ext2_fd();
unsigned int get_block_size();
unsigned int get_inode_size();
int read_inode(uint32_t inum, ext2_inode *inode_buffer_target);

uint32_t start_get_inode(const char* path);
#endif