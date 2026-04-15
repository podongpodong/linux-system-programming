#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<dirent.h>
#include<fcntl.h>
#include<time.h>
#include<limits.h>
#include <syslog.h> 

#include "parse_option.h"
#include "arrange.h"

List* files_to_arrange;
struct OptionInfo opInfo;

//._arranged를 제외한 숨김파일 필터링
int filter_hidden(const struct dirent *entry) {
    return (entry->d_name[0] != '.' || entry->d_name[1] == '_');
}

void arrange(struct OptionInfo* opInfo) {
	//파일 정보를 담을 연결리스트 초기화
	files_to_arrange = list_init();
	traverse_dir(opInfo->monitoring_path);

	//exclude_path안에 있는 디렉터리 제외
	if (opInfo->exclude_path_cnt > 0) {
		for (int i = 0; opInfo->exclude_path[i] != NULL; i++) {
			Node* cur = files_to_arrange->begin->next;
			while(cur != files_to_arrange->end) {
				if(!strncmp(opInfo->exclude_path[i], cur->path, strlen(opInfo->exclude_path[i]))) {
					Node* del_node = cur;
					cur = cur->next;
					free(list_delete(files_to_arrange, del_node));
				}
				else {
					cur = cur->next;
				}
			}
		}
	}

	//extensions로 입력받은 확장자를 제외하고 리스트에서 제외한다.
	if (strncmp(opInfo->extensions[0], "all", 3) != 0) {
		Node* cur = files_to_arrange->begin->next;
		while(cur != files_to_arrange->end) {
			bool is_include = 0;
			for (int i = 0; i < opInfo->extensions_cnt; i++) {
				if(strcmp(cur->extension, opInfo->extensions[i]) == 0) {
					is_include = 1;
					break;
				}
			}

			if(is_include) {
				cur = cur->next;
			}
			else {
				Node* del_node = cur;
				cur = cur->next;
				free(list_delete(files_to_arrange, del_node));
			}
		}
	}	

	//동일한 이름의 파일 정리
	remove_same_name(opInfo);
	if (move_file(files_to_arrange, opInfo) < 0) {
        perror("path truncated");
        return;
    }
	adjust_log_file(opInfo);
	freeNode(files_to_arrange);
}

void traverse_dir(char* dir_path) {
	struct dirent **file_list = NULL;
	int file_cnt = 0;

	file_cnt = scandir(dir_path, &file_list, filter_hidden, alphasort);

	for(int i = 0; i<file_cnt; i++) {

		if (strncmp(file_list[i]->d_name, "ssu_cleanupd.log", 16) == 0) continue;
		if (strncmp(file_list[i]->d_name, "ssu_cleanupd.config", 19) == 0) continue;
		
		Node* new_node = (Node*)malloc(sizeof(Node));

		char cur_path[PATH_MAX];
		snprintf(cur_path, sizeof(cur_path), "%s/%s", dir_path, file_list[i]->d_name);

		if(realpath(cur_path, new_node->path) < 0) {
			perror("realpath");
			free(file_list[i]);
			continue;
		}

		struct stat statbuf;
		if(stat(cur_path, &statbuf) != 0) {
			perror("stat");
			free(file_list[i]);
			continue;
		}
		
		if(S_ISDIR(statbuf.st_mode)) {
			new_node->is_dir = true;
			for(int j = 0; opInfo.exclude_path[j] != NULL; j++) {
				strcpy(opInfo.exclude_path[j], new_node->path);
				break;
			}
		}

		//파일의 이름을 확장자와 이름으로 분리
		char* dot = strrchr(file_list[i]->d_name, '.');

		if(dot != NULL && dot != file_list[i]->d_name) {
			size_t name_len = dot - file_list[i]->d_name;
			strncpy(new_node->name, file_list[i]->d_name, name_len);
			new_node->name[name_len] = '\0';
			
			if (*(dot+1) != '\0')
				strcpy(new_node->extension, dot+1);
			else
				new_node->extension[0] = '\0';

		}
		else {
			strcpy(new_node->name, file_list[i]->d_name);
			new_node->extension[0] = '\0';
		}
		
		new_node->mtime = statbuf.st_mtime;
		new_node->is_checked = false;

		//디렉터리라면 재귀호출
		if (S_ISDIR(statbuf.st_mode)) {
			traverse_dir(cur_path);
		}

		free(file_list[i]);

		insert_front(files_to_arrange, new_node);
	}
	free(file_list);
}

void remove_same_name(struct OptionInfo* opInfo) {
	Node* temp_node = NULL;

	Node* cur = files_to_arrange->begin->next;
	while (cur != files_to_arrange->end) {
		if (cur->is_dir) {
			cur = cur->next;
			continue;
		}

		if (cur->is_checked) {
			cur = cur->next;
			continue;
		}
		
		Node* best_node = cur;
		temp_node = cur->next;

		//config에 설정한 mode값에 따라서 동일 이름 파일을 정리
		bool has_dup = false;
		while (temp_node != files_to_arrange->end) {
			if (temp_node->is_dir || temp_node->is_checked) {
				temp_node = temp_node->next;
				continue;
			}

			if (!strcmp(cur->name, temp_node->name) && !strcmp(cur->extension, temp_node->extension)) {
				has_dup = true;
				bool update = false;

				if(opInfo->mode_number == 1) {
					if(temp_node->mtime > best_node->mtime)
						update = true;
				}
				else if(opInfo->mode_number == 2) {
					if(temp_node->mtime < best_node->mtime)
						update = true;
				}

				if(update) {
					best_node = temp_node;
				}

				temp_node->is_checked = true;
			}

			temp_node = temp_node->next;
		}

		if (opInfo->mode_number == 3 && has_dup) {
			cur = cur->next;
			continue;
		}
		
		Node* curr = files_to_arrange->begin->next;
		while(curr != files_to_arrange->end) {
			Node* next =curr->next;

			if (!strcmp(cur->name, curr->name) && !strcmp(cur->extension, curr->extension)) {
				if ( curr != best_node) {
					free(list_delete(files_to_arrange, curr));
				}
			}

			curr = next;
		}

		cur = cur->next;
	}
}

int move_file(const List* files_to_arrange, struct OptionInfo* opInfo) {
	Node *node = files_to_arrange->begin->next;
    DIR *dir;

	if (mkdir(opInfo->output_path, 0755)<0) {
	};
	while (node != files_to_arrange->end) {
        if (node->is_dir) {
            node = node->next;
            continue;
        }

        char dir_path[PATH_MAX], dst_path[PATH_MAX];
        if (snprintf(dir_path, sizeof(dir_path), "%s/%s", opInfo->output_path, node->extension) > sizeof(dir_path)) {
            return -1;
        }
		if (node->extension[0] == '\0') {
			if (snprintf(dst_path, sizeof(dst_path), "%s%s", dir_path, node->name) > sizeof(dst_path)) {
            	return -1;
        	}
		}
		else {
			if (snprintf(dst_path, sizeof(dst_path), "%s/%s.%s", dir_path, node->name, node->extension) > sizeof(dst_path)) {
            	return -1;
        	}
		}

        if ((dir = opendir(dir_path)) == NULL) {
            mkdir(dir_path, 0755);
        } else {
            closedir(dir);
        }
		
		//이동할 파일을 copy_file로 복사하고 log파일에 기록한다.
		struct stat stat_file;
		if (stat(dst_path, &stat_file) < 0) {
        	copy_file(node->path, dst_path);
			write_log(opInfo, node->path, dst_path);
		}
		else {
			if (opInfo->mode_number == 1) {
				if(stat_file.st_mtime < node->mtime) {
        			copy_file(node->path, dst_path);
					write_log(opInfo, node->path, dst_path);
				}
			}
			else if (opInfo->mode_number == 2) {
				if(stat_file.st_mtime > node->mtime) {
        			copy_file(node->path, dst_path);
					write_log(opInfo, node->path, dst_path);
				}
			}
			else if (opInfo->mode_number == 3) {
			}

		}
        node = node->next;
    }

    return 0;
}

void copy_file(const char *src, const char *dst)
{
    int src_fd;
    if ((src_fd = open(src, O_RDONLY)) < 0) {
        perror("open error");
        return;
    }

    int dst_fd;
    if ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        perror("open error");
        return;
    }

    char buf[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dst_fd, buf, bytes_read) != bytes_read) {
            perror("write error");
            close(src_fd);
            close(dst_fd);
        }
    }

    close(src_fd);
    close(dst_fd);
}

void write_log(struct OptionInfo *opInfo, char *src_path, char *dst_path) {
	char log_file_path[PATH_MAX];
	if (snprintf(log_file_path, sizeof(log_file_path), "%s/ssu_cleanupd.log", opInfo->monitoring_path) > sizeof(log_file_path)) {
		fprintf(stderr, "Error: Path length exceeds the maximum limit of 4096 bytes.");
		return;
	}

	FILE *fp;
	if ((fp=fopen(log_file_path, "a+")) == NULL) {
	}

	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);
	char time_buf[10];
	strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

	//파일에 로그를 기록한다.
	fprintf(fp, "[%s] [%d] [%s] [%s]\n", time_buf, getpid(), src_path, dst_path);
	fflush(fp);

	//config파일에 -l옵션이 설정되었다면 로그의 길이를 조절한다.
	if (opInfo->max_log_lines > 0) {
		rewind(fp);

		char **logs = NULL;
		int log_count = 0;
		char buffer[PATH_MAX*2];

		while (fgets(buffer, sizeof(buffer), fp) != NULL) {
			logs = realloc(logs, sizeof(char *) * (log_count+1));
			logs[log_count] = strdup(buffer);
			log_count++;
		}
		fclose(fp);

		if (log_count > opInfo->max_log_lines) {
			FILE* fp_write = fopen(log_file_path, "w");
			if (fp_write == NULL) {
				for (int i = 0; i<log_count; i++)
					free(logs[i]);
				free(logs);
			}

			for (int i = log_count-opInfo->max_log_lines; i<log_count; i++)
				fputs(logs[i], fp_write);

			fclose(fp_write);
		}
		
		for (int i = 0; i<log_count; i++)
			free(logs[i]);
		free(logs);
	}
}

//정리된 파일이 없지만 -l옵션이 적용되어 로그를 정리해야하는 경우에 사용한다.
void adjust_log_file(struct OptionInfo *opInfo) {
    if (opInfo->max_log_lines <= 0)
        return;

    char log_file_path[PATH_MAX];
    if (snprintf(log_file_path, sizeof(log_file_path), "%s/ssu_cleanupd.log", opInfo->monitoring_path) > sizeof(log_file_path)) {
        fprintf(stderr, "Error: Path length exceeds limit.\n");
        return;
    }

    FILE *fp = fopen(log_file_path, "r");
    if (fp == NULL) {
        return;
    }

    char **logs = NULL;
    int log_count = 0;
    char buffer[PATH_MAX * 2];

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        logs = realloc(logs, sizeof(char *) * (log_count + 1));
        logs[log_count++] = strdup(buffer);
    }
    fclose(fp);

    if (log_count > opInfo->max_log_lines) {
        FILE *fp_write = fopen(log_file_path, "w");
        if (fp_write == NULL) {
            for (int i = 0; i < log_count; i++)
                free(logs[i]);
            free(logs);
            return;
        }

        for (int i = log_count - opInfo->max_log_lines; i < log_count; i++) {
            fputs(logs[i], fp_write);
        }

        fclose(fp_write);
    }

    for (int i = 0; i < log_count; i++)
        free(logs[i]);
    free(logs);
}

//디버깅용 함수이다.
void print_list(List *list)
{
    FILE *log_fp = fopen("/home/minuk/my_daemon_log.txt", "a");
    if (log_fp == NULL) {
        perror("fopen log file");
        return;
    }

    Node *node = list->begin->next;
    while (node != list->end) {
        if (node->is_dir) {
            node = node->next;
            continue;
        }
        fprintf(log_fp, "file : [%s].[%s], path : [%s]\n", node->name, node->extension, node->path);
        node = node->next;
    }

	remove("/home/minuk/my_daemon_log.txt");
    fclose(log_fp);
}


List* list_init() {
	List* list = (List*)malloc(sizeof(List));
	list->begin = list->end = &list->dummy;
	list->begin->next = list->end;
	list->end->prev = list->begin;
	list->head = list->tail = NULL;
	list->size = 0;
	return list;
}

void insert_front(List* list, Node* node) {
	list->size++;

	node->prev = list->begin;
	node->next = list->begin->next;

	list->begin->next->prev = node;
	list->begin->next = node;
}

void insert_back(List* list, Node* node) {
	list->size++;

	node->next = list->end;
	node->prev = list->end->prev;

	list->end->prev->next = node;
	list->end->prev = node;
}

Node* list_delete(List* list, Node* node) {
	list->size--;

	node->prev->next = node->next;
	node->next->prev = node->prev;

	node->prev = node->next = NULL;

	return node;
}

Node* copy_node(Node* node) {
	Node* ret_node = (Node*)malloc(sizeof(Node));

	strcpy(ret_node->name, node->name);
	strcpy(ret_node->path, node->path);
	strcpy(ret_node->extension, node->extension);

	ret_node->mtime = node->mtime;
	ret_node->is_dir = node->is_dir;
	ret_node->is_checked = node->is_checked;

	ret_node->next = ret_node->prev = NULL;

	return ret_node;
}

void freeNode(List* list) {
	Node *cur = list->begin->next;
	while(cur != list->end) {
		Node *cur_next = cur->next;
		free(cur);
		cur = cur_next;
	}

	list->begin = &list->dummy;
	list->end = &list->dummy;
}
