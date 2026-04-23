#ifndef SSU_EXT2_H
#define SSU_EXT2_H

#include "ext2_struct.h"

int load_ext2_image(char *path);
int read_inode(uint32_t inum, ext2_inode *inode_buffer_target);

uint32_t start_get_inode(const char* path);
#endif