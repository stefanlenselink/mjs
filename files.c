#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "files.h"
#include "misc.h"
#include "info.h"
#include "extern.h"

static int	sort_mp3(const void *, const void *);
static int	sort_mp3_search(const void *, const void *);
static int	check_file(flist *);

flist *
read_mp3_list(wlist *list)
{
	char *dir = NULL;
	DIR *dptr = NULL;
	struct dirent *dent;
	struct stat st;
	flist *ftmp = NULL, *mp3list = list->head;
	int length = 0;

	list->where = 1;
	list->wheretop = 1;
	dir = getcwd(NULL, 0);
	errno = 0;
	if (errno) {
		my_mvwprintw(menubar->win, 0, 0, colors[MENU_TEXT], "Error with opendir - Pleurt op !(): %s", strerror(errno)); 
		return NULL;
		}

	if (strncmp(dir,conf->mp3path,8)){
		chdir(conf->mp3path);
		dir = getcwd(NULL, 0);
		}
	else {
		if (strcmp(dir,conf->mp3path)) {
			ftmp = calloc(1, sizeof(flist));
			ftmp->flags |= F_DIR;
			ftmp->filename = strdup("../");
			ftmp->fullpath = strdup("../");
			ftmp->path = strdup(dir);
			ftmp->next = mp3list;
			if (mp3list)
				mp3list->prev = ftmp;
			mp3list = ftmp;
			length++;
			}
		}
		
	dptr = opendir(dir);
		
	while ((dent = readdir(dptr))) {
		if (*dent->d_name == '.')
			continue;
		stat(dent->d_name, &st);
		if (S_ISDIR(st.st_mode)) {
			ftmp = calloc(1, sizeof(flist));
			ftmp->flags |= F_DIR;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
			ftmp->filename = malloc(dent->d_namlen+3);
#else
			ftmp->filename = malloc(strlen(dent->d_name)+3);
#endif /* *BSD */
			ftmp->filename[0]='/';
			strcpy(ftmp->filename+1, dent->d_name);
			strcat(ftmp->filename, "/");

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
			ftmp->fullpath = calloc(1, strlen(dir)+dent->d_namlen+2);
#else
			ftmp->fullpath = calloc(1, strlen(dir)+strlen(dent->d_name)+2);
#endif /* *BSD */
			sprintf(ftmp->fullpath, "%s/%s", dir, dent->d_name);
			ftmp->path = strdup(dir);

			ftmp->next = mp3list;
			if (mp3list)
				mp3list->prev = ftmp;
			mp3list = ftmp;
			length++;
		} else if (S_ISREG(st.st_mode)) {

			if ((strncasecmp(".mp3", strchr(dent->d_name, '\0')-4, 4)) && (strncasecmp(".mms", strchr(dent->d_name, '\0')-4, 4)))
				continue;
			if (strncasecmp(".mp3", strchr(dent->d_name, '\0')-4, 4)){
				ftmp = calloc(1, sizeof(flist));
				ftmp->flags |= F_PLAYLIST;

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
				ftmp->filename = malloc(dent->d_namlen-1);
#else
				ftmp->filename = malloc(strlen(dent->d_name)-1);
#endif /* *BSD */
				ftmp->filename[0]='<';
				strncpy(ftmp->filename+1, dent->d_name, strlen(dent->d_name)-4);
				ftmp->filename[strlen(dent->d_name)-3]='\0';
				strcat(ftmp->filename,">\0");
			}
			else {
				ftmp = NULL;
				if (!(ftmp = mp3_info(dent->d_name, ftmp, st.st_size)))
					continue;	
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
				ftmp->filename = malloc(dent->d_namlen-3);
#else
				ftmp->filename = malloc(strlen(dent->d_name)-3);
#endif /* *BSD */
				strncpy(ftmp->filename, dent->d_name, strlen(dent->d_name)-4);
				ftmp->filename[strlen(dent->d_name)-4]='\0';
			}
			ftmp->path = strdup(dir);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
			ftmp->fullpath = calloc(1, strlen(dir)+dent->d_namlen+2);
#else
			ftmp->fullpath = calloc(1, strlen(dir)+strlen(dent->d_name)+2);
#endif /* *BSD */
			sprintf(ftmp->fullpath, "%s/%s", dir, dent->d_name);

			ftmp->next = mp3list;
			if (mp3list)
				mp3list->prev = ftmp;
			mp3list = ftmp;
			length++;
		}
	}
	closedir(dptr);
	free(dir);
	list->length = length;
	return ftmp;
}

int
write_mp3_list_file(wlist *list, char *filename)
{
	FILE *fp;
	flist *ftmp;
	
	if (!(fp = fopen(filename,"w")))
		return 1;
	fprintf(fp,"Playlist for mms\n");
	for (ftmp = list->head; ftmp; ftmp=ftmp->next)
		fprintf(fp,"%s\n",ftmp->fullpath);
	fclose(fp);
	return 0;
}


flist *
read_mp3_list_file(wlist *list, char *filename)
{
	char *buf = NULL;
	char *file = NULL;
	FILE *fp;
	char *dir = NULL;
	char * playlistname = NULL;
	struct stat st;
	flist *ftmp = NULL, *tail = NULL, *mp3list = list->tail;
	int length = 0, lengte = 0, n = 0;
	int playlist = 0;
	
	list->where = 1;
	errno=0;
	if (!(fp = fopen(filename,"r")))
		return mp3list;

	buf = calloc(256, sizeof(char));

	ftmp = calloc(1, sizeof(flist));
	ftmp->flags |= F_SEARCHDIR | F_DIR;
	ftmp->filename = strdup("../");
//	ftmp->fullpath = strdup("../");
	ftmp->fullpath = getcwd(NULL,0);
//	ftmp->path = strdup("../");
	ftmp->path = strdup(ftmp->fullpath);
	if (mp3list) {
		ftmp->prev = list->tail;
		list->tail->next=ftmp;
	} else 
		list->head = ftmp;
	tail = ftmp;
	length++;
	fgets(buf, 255, fp);
	if (!strncmp("Playlist for mms",buf,16))
		playlist = 1;
	else
		if (strncmp("Findresults for mms",buf,19)) {
			fclose(fp);
			free(buf);
			return ftmp;
		}
	if (playlist){
		lengte = strrchr(filename,'/')-filename;
		playlistname = malloc(strlen(filename)-lengte-4);
		strncpy(playlistname, filename+lengte+1,strlen(filename)-lengte-5);
		playlistname[strlen(filename)-lengte-5]='\0';
	}
			
	while (!feof(fp)) {
		n++;
		if (n & 0x03){
			my_mvwaddstr(menubar->win, 0, (24+(n/4)), colors[MENU_TEXT], "*");
			update_panels();
			doupdate();
			}
		if (!fgets(buf, 255, fp)){
			// end-of-file reached or got zero characters
			fclose(fp);
			free(buf);	
			free(playlistname);
			list->length = length;
			return list->head;
		}
		lengte=strrchr(buf,'/')-buf;
		buf[strlen(buf)-1]='\0';		// Get rid off trailing newline
		if (buf=='\0')
			goto endloop;
		dir = malloc(lengte+2);
		file = malloc(strlen(buf)-lengte);		
		strncpy(dir, buf, lengte+1);
		dir[lengte+1]='\0';
		strcpy(file, buf+lengte+1);

		stat(buf, &st);
		if (S_ISDIR(st.st_mode)) {
			ftmp = calloc(1, sizeof(flist));
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
			ftmp->filename = malloc(file+3);
#else
			ftmp->filename = malloc(strlen(file)+3);
#endif /* *BSD */
			strcpy(ftmp->filename, file);
			strcat(ftmp->filename, ":");
			ftmp->fullpath = strdup(buf);
			ftmp->path = strdup(buf);
			ftmp->flags |= F_SEARCHDIR | F_DIR;
			tail->next = ftmp;
			ftmp->prev = tail;
			tail = ftmp;
			length++;
			} 
		else 
			if (S_ISREG(st.st_mode)) {
/* FIXME (goto's) */
				if (strncasecmp(".mp3", strchr(file, '\0')-4, 4))
					goto endloop;
				ftmp = NULL;
				if (!(ftmp = mp3_info(buf, ftmp, st.st_size)))
					goto endloop;

				if (playlist) {
					free(ftmp->album);
					ftmp->album = strdup(playlistname);
					if ((file[0]=='0')|(file[0]=='1')|(file[0]=='2')) {
						// get rid of old tracknumber add new tracknumber
						ftmp->filename = malloc(strlen(file)-3);
						snprintf(ftmp->filename, 3, "%02.0f", (float)length);
						ftmp->filename[2]=' ';
						strncpy(ftmp->filename+3, file+3, strlen(file)-7);
						ftmp->filename[strlen(file)-4]='\0';

					} else {
						// get rid of .mp3 add tracknumber
						ftmp->filename = malloc(strlen(file));
						snprintf(ftmp->filename, 3, "%02.0f", (float)length);
						ftmp->filename[2]=' ';
						strncpy(ftmp->filename+3, file, strlen(file)-4);
						ftmp->filename[strlen(file)-1]='\0';
					}

				} else {
					// get rid of .mp3
					ftmp->filename = malloc(strlen(file)-3);
					strncpy(ftmp->filename, file, strlen(file)-4);
					ftmp->filename[strlen(file)-4]='\0';
				}

				ftmp->path = strdup(dir);
				ftmp->fullpath = strdup(buf);

				tail->next = ftmp;
				ftmp->prev = tail;
				tail = ftmp;
				length++;
				}
endloop:
		free(file);
		free(dir);
		}
	fclose(fp);
	free(buf);
	free(playlistname);
	list->length = length;
	return list->head;

}

/* sort directories first, then files. alphabetically of course. */
wlist *
sort_songs(wlist *sort)
{
	int i = 0, j;
	flist *ftmp = NULL, *newlist = NULL, **fsort;
	
	for (ftmp = sort->head; ftmp; ftmp = ftmp->next)
		i++; 
	fsort = (flist **) calloc(i, sizeof(flist *));
	for (ftmp = sort->head, j = 0; ftmp; ftmp = ftmp->next, j++)
		fsort[j] = ftmp;

	qsort((void *)fsort, i, sizeof(flist *), sort_mp3);
	sort->tail = newlist = fsort[0];
	newlist->next = NULL;

	for (j = 1; j < i; j++) {
		ftmp = fsort[j];
		ftmp->next = newlist;
		newlist->prev = ftmp;
		newlist = ftmp;
	}
	free(fsort);
	newlist->prev = NULL;
	sort->head = sort->top = sort->selected = newlist;
	newlist->flags |= F_SELECTED;
	return sort;
}

wlist *
sort_search(wlist *sort)
{
	int i = 0, j;
	flist *ftmp = NULL, *newlist = NULL, **fsort;
	
	for (ftmp = sort->head; ftmp; ftmp = ftmp->next)
		i++; 
	fsort = (flist **) calloc(i, sizeof(flist *));
	for (ftmp = sort->head, j = 0; ftmp; ftmp = ftmp->next, j++)
		fsort[j] = ftmp;

	qsort((void *)fsort, i, sizeof(flist *), sort_mp3_search);
	sort->tail = newlist = fsort[0];
	newlist->next = NULL;

	for (j = 1; j < i; j++) {
		ftmp = fsort[j];
		ftmp->next = newlist;
		newlist->prev = ftmp;
		newlist = ftmp;
	}
	free(fsort);
	newlist->prev = NULL;
	sort->head = sort->top = sort->selected = newlist;
	newlist->flags |= F_SELECTED;
	return sort;
}

static int
sort_mp3(const void *a, const void *b)
{
	const flist *first = *(const flist **) a;
	const flist *second = *(const flist **) b;
	if ((first->flags & F_DIR) && !(second->flags & F_DIR))
		return 1;
	else if (!(first->flags & F_DIR) && (second->flags & F_DIR))
		return -1;
	else
		return strcmp(second->filename, first->filename);
}

static int
sort_mp3_search(const void *a, const void *b)
{
	const flist *first = *(const flist **) a;
	const flist *second = *(const flist **) b;
	int result;
	result = strcmp(second->path, first->path);
	if (!result) {
		if ((first->flags & F_DIR) && !(second->flags & F_DIR))
			return 1;
		else 
			if (!(first->flags & F_DIR) && (second->flags & F_DIR))
				return -1;
			else 
				return strcmp(second->filename, first->filename);
		}
	return result;
}

flist *
delete_file(Window *win, flist *file)
{
	wlist *list = play->contents.list;
	flist *fnext, *fprev, *ftmp;
	int i, maxx, maxy;
	if (!list)
		return NULL;
	if (!file)
		return NULL;
	if (file->flags & F_PLAY)
		return file;
	maxx = win->width;
	maxy = win->height - 3;
	free_flist(file);
	list->length--;
	if ((fnext = file->next)) {
		if ((fprev = file->prev)) {
			fprev->next = fnext;
			fnext->prev = fprev;
		} else {
			fnext->prev = NULL;
			list->head = fnext;
		}
		if (file->flags & F_SELECTED) 
		 	fnext->flags |= F_SELECTED;
		if (file == list->top)
			list->top = fnext;
	 	free(file);
	 	ftmp = fnext;
	} else 
		if ((fprev = file->prev)) {
			fprev->next = NULL;
			list->tail = fprev;
			list->where--;
			if (file->flags & F_SELECTED) 
				fprev->flags |= F_SELECTED;
			if (list->top == file) {
				for (i = 0, ftmp = fprev; ftmp && ftmp->prev && (i < maxy); ftmp = ftmp->prev, i++);
				list->top = ftmp;
			}
			free(file);
			ftmp = fprev;
		} else {
			free(file);
			list->head = list->top = list->tail = NULL;
			list->where = 0;
			ftmp = NULL;
			}

	return (list->selected = ftmp);
}

static int
check_file(flist *file)
{
	struct stat sb;

	if (stat(file->fullpath, &sb) == -1)
		return 0;
	else
		return 1;
}

/* find the next valid entry in the search direction */

flist *
next_valid(Window *win, flist *file, int c)
{
	flist *ftmp = file;
	int fix_selected = 0;
	if (!file)
		return file;
	if (file->flags & F_SELECTED)
		fix_selected = 1;
	switch (c) {
		case KEY_HOME:
		case KEY_DOWN:
		case KEY_NPAGE:
			while (!check_file(file))
				file = delete_file(win, file);
			break;
		case KEY_END:
		case KEY_UP:
		case KEY_PPAGE:
			while (!check_file(file)) {
				ftmp = file->prev;
				delete_file(win, file);
				file = ftmp;
			}
			break;
	}
	if (fix_selected) {
		win->contents.list->selected = file;
		file->flags |= F_SELECTED;
	}
	return file;

}
