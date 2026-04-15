#include<stdio.h>
#include<string.h>

#include "help.h"

void help(char* command) {
	printf("Usage:\n");
	if(command == NULL) {
		help_show();
		help_add();
		help_modify();
		help_remove();
		help_help();
		help_exit();
	}
	else if (strcmp(command, "show") == 0) {
		help_show();
	}
	else if (strcmp(command, "add") == 0) {
		help_add();
	}
	else if (strcmp(command, "modify") == 0) {
		help_modify();
	}
	else if (strcmp(command, "remove") == 0) {
		help_remove();
	}
	else if (strcmp(command, "help") == 0) {
		help_help();
	}
	else if (strcmp(command, "exit") == 0) {
		help_exit();
	}
}

void help_show() {
	printf(" > show\n");
	printf("   <none> : show monitoring deamon process info\n");
}

void help_add() {
	printf(" > add <DIR_PATH> [OPTION]...\n");
	printf("   <none> : add deamon process monitoring the <DIR_PATH> directory\n");
	printf("   -d <OUTPUT_PATH> : Specify the output directory <OUTPUT_PATH> where <DIR_PAT> will be arranged\n");
	printf("   -i <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n");
	printf("   -l <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record.\n");
	printf("   -x <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n");
	printf("   -e <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n");
	printf("   -m <M> : Specify the value for the <M> option.\n");
}

void help_modify() {
	printf(" > modify <DIR_PATH> [OPTION]...\n");
	printf("   <none> : modify deamon process config monitoring the <DIR_PATH> directory\n");
	printf("   -d <OUTPUT_PATH> : Specify the output directory <OUTPUT_PATH> where <DIR_PATH> will be arranged\n");
	printf("   -i <TIME_INTERVAL> : Set the time interval for the daemon process to monitor in seconds.\n");
	printf("   -l <MAX_LOG_LINES> : Set the maximum number of log lines the daemon process will record.\n");
	printf("   -x <EXCLUDE_PATH1, EXCLUDE_PATH2, ...> : Exclude all subfiles in the specified directories.\n");
	printf("   -e <EXTENSION1, EXTENSION2, ...> : Specify the file extensions to be organized.\n");
	printf("   -m <M> : Specify the value for the <M> option.\n");
}

void help_remove() {
	printf(" > remove <DIR_PATH>\n");
	printf("   <none> : remove deamon process monitoring the <DIR_PATH> directory\n");
}

void help_help() {
	printf(" > help\n");
}

void help_exit() {
	printf(" > exit\n");
}
