#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "unistd.h"
#include "stdio.h"

int
find(char *nieuwedir, int diepte)
{
	char *dir = NULL;
	DIR *dptr = NULL;
	struct dirent *dent;
	struct stat st;
	pid_t childpid;
	static struct sigaction handler;
	char *filep = NULL;
	diepte++;

	chdir(nieuwedir);
	errno = 0;
	dir = getcwd(NULL, 0);
	dptr = opendir(dir);
		
	
	while ((dent = readdir(dptr))) {
		if ((*dent->d_name == '.') && strcmp(dent->d_name, ".."))
			continue;
		stat(dent->d_name, &st);
		if (S_ISDIR(st.st_mode)) { 

			if (!(strncmp(dent->d_name, ".",1)))
				continue;
			if (diepte < 3) {
				find(dent->d_name, diepte);
				chdir(dir);
			}
			if (diepte==3) {
				printf("%s\n",dir);	
				return;
			}
		}
	}
	
	closedir(dptr);
	free(dir);
	return 0;
}

int 
main(int argc, char *argv[])
{
	find(getcwd(NULL, 0), 0);
	return 0;
}
