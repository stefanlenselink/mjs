#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "files.h"
#include "misc.h"
#include "info.h"
#include "extern.h"

static int	sort_mp3(const void *, const void *);
static int	check_file(flist *);

flist *
read_mp3_list(wlist *list)
{
	char *dir = NULL;
	DIR *dptr = NULL;
	struct dirent *dent;
	struct stat st;
	flist *ftmp = NULL, *mp3list = list->head, *tmp = NULL;
	int length = 0;

	list->where = 1;
	dir = getcwd(NULL, 0);
	errno = 0;
	dptr = opendir(dir);
	if (errno) {
		my_mvwprintw(menubar->win, 0, 0, colors[MENU_TEXT], "Error with opendir(): %s", strerror(errno));
		return NULL;
	}
	while ((dent = readdir(dptr))) {
		if ((*dent->d_name == '.') && strcmp(dent->d_name, ".."))
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
			ftmp->colors = colors[UNSELECTED];

			ftmp->next = mp3list;
			if (mp3list)
				mp3list->prev = ftmp;
			mp3list = ftmp;
			length++;
		} else if (S_ISREG(st.st_mode)) {
			if (strncasecmp(".mp3", strchr(dent->d_name, '\0')-4, 4))
				continue;
			tmp = NULL;
			if (!(tmp = mp3_info(dent->d_name, tmp, st.st_size)))
				continue;
			ftmp = tmp;
			ftmp->filename = strdup(dent->d_name);

			ftmp->path = strdup(dir);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
			ftmp->fullpath = calloc(1, strlen(dir)+dent->d_namlen+2);
#else
			ftmp->fullpath = calloc(1, strlen(dir)+strlen(dent->d_name)+2);
#endif /* *BSD */
			sprintf(ftmp->fullpath, "%s/%s", dir, ftmp->filename);
			ftmp->colors = colors[UNSELECTED];

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
	if (newlist->flags & F_PLAY)
		newlist->colors = colors[SEL_PLAYING];
	else
		newlist->colors = colors[SELECTED];
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

flist *
delete_file(Window *win, flist *file)
{
	wlist *list = win->contents.list;
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
		if (file->flags & F_SELECTED) {
		 	fnext->flags |= F_SELECTED;
			if (fnext->flags & F_PLAY)
				fnext->colors = colors[SEL_PLAYING];
			else
				fnext->colors = colors[SELECTED];
		}
		if (file == list->top)
			list->top = fnext;
	 	free(file);
	 	ftmp = fnext;
	} else if ((fprev = file->prev)) {
		fprev->next = NULL;
		list->tail = fprev;
		list->where--;
		if (file->flags & F_SELECTED) {
			fprev->flags |= F_SELECTED;
			if (fprev->flags & F_PLAY)
				fprev->colors = colors[SEL_PLAYING];
			else
				fprev->colors = colors[SELECTED];
		}
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
