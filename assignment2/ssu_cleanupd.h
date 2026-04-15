#ifndef SSU_CLEANUPD_H
#define SSU_CLEANUPD_H

#define MAX_ARGS 5000

extern void modify();

extern char* parse_command[MAX_ARGS];
extern int parse_count;
extern FILE* fd_daemon_list;
extern char* daemon_list_path;
// Initialize ssu_cleanupd: create ~/.ssu_cleanupd and current_deamon_list file if it is not exist
void init_ssu_cleanupd();
//Tokenizes the input string by space.
void tokenize_input(char* buffer);
void convert_tilde(char **path);

#endif
