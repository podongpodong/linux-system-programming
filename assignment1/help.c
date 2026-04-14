#include<stdio.h>
#include<string.h>
#include "util.h"

static void help_tree(void) 
{
	printf(" > tree <DIR_PATH> [OPTION]...\n");
	printf("   <none> : Display the directory structure recursively if <DIR_PATH> is a directory\n");
	printf("   -s : Display the directory structure recursively if <DIR_PATH> is a directory, including the size of each file\n");
	printf("   -p : Display the directory structure recursively if <DIR_PATH> is a directory, including the permissions of each directory and file\n");
}

static void help_arrange(void) 
{
	printf(" > arrange <DIR_PATH> [OPTION]...\n");
	printf("   <none> : Arrange the directory if <DIR_PATH> is a directory\n");
	printf("   -d <output_path> : Specify the output directory <output_path> where <DIR_PATH> will be arranged if <DIR_PATH> is a directory\n");
	printf("   -t <seconds> : Only arrange files that were modified more than <seconds> seconds ago\n");
	printf("   -x <exclude_path1, exclude_path2, ...> : Arrange the directory if <DIR_PATH> is a directory except for the files inside <exclude_path> directory\n");
	printf("   -e <extension1, extension2, ...> : Arrange the directory with the specified extension <extension1, extension2, ...>\n");
}

static void help_help(void) 
{
	printf(" > help [COMMAND]\n");
}

static void help_exit(void) 
{
	printf(" > exit\n");
}

void show_help(char *command) {
	printf("Usage:\n");

	if(command == NULL) {
		help_tree();
		help_arrange();
		help_help();
		help_exit();
	}
	else if(strcmp(command, "tree") == 0) {
		help_tree();
	}
	else if(strcmp(command, "arrange") == 0) {
		help_arrange();
	}
	else if(strcmp(command, "help") == 0) {
		help_help();
	}
	else if(strcmp(command, "exit") == 0) {
		help_exit();
	}
}
