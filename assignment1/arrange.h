#ifndef ARRANGE_H
#define ARRANGE_H

#include "tree.h"
#include "hashtable.h"

//start arrange command
void arrange();

//find all file under <DIR_PATH> 
void find_file(char* dir_path, char* path);

//arrnage option -x
//checking exclude path
int exclude_dir(char* dir_name);

//arrange option -e
//checking include extenstion
int include_file(char* fname);

//arrange file for extension
void arrange_file();

//option parse arrange
void arrange_option();

//copy file
void copy_file(char *src, char *dest);

//parse after .
char* get_extension(char* file_name);

//if exist same filename in arrange choose option
void option_choosen(HashNode* cur, char* dest_path);

//run vi diff 
void option_proc(char** option);

//return num1, num2 idx
PathNode* get_file(PathNode* cur_path, int idx);

#endif
