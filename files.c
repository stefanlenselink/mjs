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
	flist *ftmp = NULL;

	list->where = 0;
	list->wheretop = 0;
	list->length = 0;
	list->flags &= ~F_VIRTUAL;
	dir = getcwd(NULL, 0);
	errno = 0;
	if (errno) {
		my_mvwprintw(menubar->win, 0, 0, colors[MENU_TEXT], "Error with opendir(): %s", strerror(errno)); 
		return NULL;
		}

	if ((strncmp(dir,conf->mp3path,strlen(conf->mp3path))) && (strcmp(dir,conf->playlistpath))){
		chdir(conf->mp3path);
		dir = getcwd(NULL, 0);
	} else 	if (strcmp(dir,conf->mp3path)) {
		ftmp = calloc(1, sizeof(flist));
		ftmp->flags |= F_DIR;
		ftmp->filename = strdup("../");
		if (strcmp(dir,conf->playlistpath))
			ftmp->fullpath = strdup("../");
		else
			ftmp->fullpath = strdup(conf->mp3path);
		ftmp->path = strdup(dir);

		ftmp->next = NULL;
		ftmp->prev = NULL;
		list->head = list->tail = ftmp;
		
		list->length++;
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
			ftmp->filename = malloc(dent->d_namlen+2);
#else
			ftmp->filename = malloc(strlen(dent->d_name)+2);
#endif /* *BSD */

			
			strcpy(ftmp->filename, dent->d_name);
			strcat(ftmp->filename, "/");


#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
			ftmp->fullpath = calloc(1, strlen(dir)+dent->d_namlen+2);
#else
			ftmp->fullpath = calloc(1, strlen(dir)+strlen(dent->d_name)+2);
#endif /* *BSD */

			
			sprintf(ftmp->fullpath, "%s/%s", dir, dent->d_name);
			ftmp->path = strdup(dir);

		} else if (S_ISREG(st.st_mode)) {

			if ((strncasecmp(".mp3", strchr(dent->d_name, '\0')-4, 4)) && (strncasecmp(".mjs", strchr(dent->d_name, '\0')-4, 4)))
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
				if (!(ftmp = mp3_info(dir, dent->d_name, ftmp, st.st_size)))
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

		}
	
		ftmp->prev = list->tail;
		ftmp->next = NULL;

		if (list->tail)
			list->tail->next = ftmp;
		list->tail = ftmp;
		if (!list->head)
			list->head = ftmp;
		
		list->length++;
}
	closedir(dptr);
	free(dir);
	list->selected = list->top = list->head;
	return list->head;
}

int
write_mp3_list_file(wlist *list, char *filename)
{
	FILE *fp;
	flist *ftmp;
	
	if (!(fp = fopen(filename,"w")))
		return 1;
	fprintf(fp,"Playlist for mjs\n");
	for (ftmp = list->head; ftmp; ftmp=ftmp->next)
		fprintf(fp,"%s\n",ftmp->fullpath);
	fclose(fp);
	return 0;
}


wlist *
read_mp3_list_file(wlist *list, char *filename)
{
	char *buf = NULL;
	char *file = NULL;
	FILE *fp;
	char *dir = NULL;
	char * playlistname = NULL;
	struct stat st;
	flist *ftmp = NULL, *tail = NULL;
	int length = 0, n = 0;
	int playlist = 0;
	
	errno=0;
	if (!(fp = fopen(filename,"r")))
		return list;

	buf = calloc(256, sizeof(char));

	//length++;
	fgets(buf, 255, fp);
	if (!strncmp("Playlist for mjs",buf,16))
		playlist = 1;

	if (playlist){
		length = strrchr(filename,'/')-filename;
		playlistname = malloc(strlen(filename)-length-4);
		strncpy(playlistname, filename+length+1,strlen(filename)-length-5);
		playlistname[strlen(filename)-length-5]='\0';
	}

	if (list) {
		if (list->tail) 
		// we've got an existing list, append the new entries
			tail = list->tail;
	} else {
		// no existing list, make a new one
		list = calloc(1, sizeof(wlist));
		list->where = 0;
		list->wheretop = 0;
		list->length = 1;
		list->flags |= F_VIRTUAL;
		ftmp = calloc(1, sizeof(flist));
		ftmp->flags |= F_DIR;
		ftmp->filename = strdup("../");
		ftmp->fullpath = getcwd(NULL,0);
		if (playlist) 
			ftmp->path = strdup(playlistname);
		else
			ftmp->path = strdup("Search Results");
		ftmp->next = NULL;
		ftmp->prev = NULL;
		list->selected = NULL;
		list->tail = NULL;
		list->head = ftmp;
		list->top = ftmp;
		tail = ftmp;
	}
			
	while (!feof(fp)) {
		n++;
		if (n & 0x03){
			my_mvwaddstr(menubar->win, 0, (26+(n/4)), colors[MENU_TEXT], "*");
			update_panels();
			doupdate();
			}
		if (!fgets(buf, 255, fp)){
			// end-of-file reached or got zero characters
			fclose(fp);
			free(buf);	
			free(playlistname);
			list->tail = tail;
			if (!list->top)
				list->top = list->head;
			if (!list->selected)
				list->selected = list->top;
			return list;
		}
		length=strrchr(buf,'/')-buf;
		buf[strlen(buf)-1]='\0';		// Get rid off trailing newline
		dir = malloc(length+1);
		file = malloc(strlen(buf)-length);		
		strncpy(dir, buf, length);
		dir[length]='\0';
		strcpy(file, buf+length+1);
		ftmp = NULL;
		if (!stat(buf, &st)) {
			if (S_ISDIR(st.st_mode)) {
// directory	
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
				ftmp->flags |= F_DIR;
				ftmp->album = NULL;
				ftmp->artist = NULL;
			} else {
				if ((S_ISREG(st.st_mode)) & ((!strncasecmp(".mp3", strchr(file, '\0')-4, 4)) && ((ftmp = mp3_info(dir, file, ftmp, st.st_size))))) {
					if (playlist) {
// playlist
						if (ftmp->album)
							free(ftmp->album);
						ftmp->album = strdup(playlistname);
						if ((file[0]>='0') & (file[0]<='9')) {
							// get rid of old tracknumber add new tracknumber
							ftmp->filename = malloc(strlen(file)-3);
							snprintf(ftmp->filename, 3, "%02.0f", (float)n);
							ftmp->filename[2]=' ';
							strncpy(ftmp->filename+3, file+3, strlen(file)-7);
							ftmp->filename[strlen(file)-4]='\0';
	
						} else {
							// get rid of .mp3 add tracknumber
							ftmp->filename = malloc(strlen(file));
							snprintf(ftmp->filename, 3, "%02.0f", (float)n);
							ftmp->filename[2]=' ';
							strncpy(ftmp->filename+3, file, strlen(file)-4);
							ftmp->filename[strlen(file)-1]='\0';
						}
	
					} else { // get rid of .mp3
// mp3-file
						ftmp->filename = malloc(strlen(file)-3);
						strncpy(ftmp->filename, file, strlen(file)-4);
						ftmp->filename[strlen(file)-4]='\0';
					
					}

					ftmp->path = strdup(dir);
					ftmp->fullpath = strdup(buf);
				}
			}
		} else {
			if (!strncasecmp(buf, "http", 4)) {
				// web-cast http adres
				ftmp = calloc(1, sizeof(flist));
				ftmp->filename = calloc(strlen(playlistname)+11, sizeof(char));
				strcpy(ftmp->filename, "WebRadio: \0");
				strcat(ftmp->filename, playlistname);
				ftmp->album = strdup(buf);
				ftmp->artist = strdup("http-stream");
				ftmp->fullpath = strdup(buf);
				ftmp->path = strdup("http-stream");
			}
		}

		if (ftmp) {
			ftmp->next = NULL;
			if (tail) { // add to the tail of the existing list
				tail->next = ftmp;
				ftmp->prev = tail;
				tail = ftmp;
			} else { // the list is empty
				ftmp->prev = NULL;
				list->head = ftmp;
				tail = ftmp;
			}
			list->length++;
		}
		free(file);
		free(dir);
		}
	fclose(fp);
	if (buf)
		free(buf);
	if (playlistname)
		free(playlistname);
	list->tail = tail;
	if (!list->top)
		list->top = list->head;
	if (!list->selected)
		list->selected = list->top;
	return list;

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

	if (first->filename[0] == '.')
		return 1;
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

	if (first->filename[0] == '.')
		return 1;
	if (!(result = strcmp(second->path, first->path))) {
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
delete_file(flist *file)
{
	wlist *list = play->contents.list;
	flist *fnext, *fprev, *ftmp;
	if (!list)
		return NULL;
	if (!file)
		return NULL;
	free_flist(file);
	list->length--;
	if ((fnext = file->next)) {
		// file is not the last file of the list
		if ((fprev = file->prev)) {
			fprev->next = fnext;
			fnext->prev = fprev;
		} else {
			fnext->prev = NULL;
			list->head = fnext;
		}
		// if file was selected make fnext selected
		if (file->flags & F_SELECTED) 
		 	fnext->flags |= F_SELECTED;
		if (file == list->top)
			list->top = fnext;
	 	free(file);
	 	ftmp = fnext;
	} else 
		// file is the last file of the list
		if ((fprev = file->prev)) {
			fprev->next = NULL;
			list->tail = fprev;
			list->where--;
			if (file->flags & F_SELECTED) 
				fprev->flags |= F_SELECTED;
			free(file);
			ftmp = fprev;
		} else {
			// no previous file, the list is empty
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
next_valid(flist *file, int c)
{
	flist *ftmp = NULL;
	if (!file)
		return NULL;
	if (!strncasecmp(file->fullpath, "http", 4))
		return file;
	switch (c) {
		case KEY_HOME:
		case KEY_DOWN:
		case KEY_NPAGE:
			while (!check_file(file))
				file = delete_file(file);
			break;
		case KEY_END:
		case KEY_UP:
		case KEY_PPAGE:
			while (!check_file(file)) {
				ftmp = file->prev;
				delete_file(file);
				file = ftmp;
			}
			break;
	}
	return file;

}
