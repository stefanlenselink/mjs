#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "files.h"
#include "misc.h"
#include "info.h"
#include "extern.h"
#include "list.h"

static int sort_mp3 (const void *, const void *);
static int sort_mp3_search (const void *, const void *);


void
leave_directory (Window * window) {
	char * filename = strdup(dirstack_filename());
	char * fullpath = strdup(dirstack_fullpath());
	wlist * list = window->contents.list;
	
	if ((dirstack_listcache())&&((dirstack_listcache())->head))
		wlist_move(list, dirstack_listcache());
	else {
		read_mp3_list (list, fullpath, L_NEW);
		while (strcmp (list->selected->filename, filename))
			move_selector (window, KEY_DOWN);
	}
	dirstack_pop();
	window->update (window);
	info->update (info);
	free(filename);
	free(fullpath);
}

void
enter_directory (Window * window, const char * fullpath){
	char *newdir = strdup(fullpath);
	wlist * list = window->contents.list;
	
	if (!(list->flags & F_VIRTUAL)) 
		dirstack_push(list->from, list->selected->filename, list);
	read_mp3_list (list, newdir, L_NEW);
	free(newdir);
}

void
read_mp3_list (wlist * list, const char * from, int append)
{
	struct stat st;
	if (!lstat (from, &st)) {

		if (S_ISLNK (st.st_mode)) {
			char tempdir[256];
			int n;
			n = readlink(from, tempdir, sizeof(tempdir));
			tempdir[n]='\0';
			list->from = strdup(tempdir);
			stat(list->from, &st);
		} else
			list->from = strdup(from);

	
		if (S_ISDIR (st.st_mode)) {
			read_mp3_list_dir (list, list->from, append);
			switch (append) {
				case L_NEW:
					if (list->head)
						sort_songs (list);
					break;
				case L_SEARCH:
					if (list->head)
						sort_search (list);
					break;
			}
		} else {
			switch (append) {
				case L_SEARCH:
					menubar->deactivate (menubar);
					printf_menubar (menubar, SEARCHING);
					break;
				default:
					menubar->deactivate (menubar);
					printf_menubar (menubar, READING);
			}
			read_mp3_list_file (list, list->from, append);
			if ((append & L_SEARCH) && (list->head))
				sort_search (list);
			menubar->activate (menubar);
		}
	}
	return;
}


void
read_mp3_list_dir (wlist * list, const char * directory, int append)
{
	char *dir = NULL;
	DIR *dptr = NULL;
	struct dirent *dent;
	flist *ftmp = NULL;
	
	
	
	dir = strdup(directory);
	wlist_clear (list);
	chdir(dir);
	
	errno = 0;
	if (errno) {
		my_mvwprintw (menubar->win, 0, 0, colors[MENU_TEXT], "Error with opendir(): %s", strerror (errno));
		return;
	}

	if ((strncmp (dir, conf->mp3path, strlen (conf->mp3path)-1)) && (strcmp (dir, conf->playlistpath))) {
		chdir (conf->mp3path);
		dir = strdup(conf->mp3path);
	} else if (!dirstack_empty()) {
		ftmp = mp3_info (dir, "../", NULL, 0, append);
		wlist_add (list, list->tail, ftmp);
	}


	dptr = opendir (dir);
	while ((dent = readdir (dptr))) {
		if (*dent->d_name == '.')
			continue;

		if ((ftmp = mp3_info (dir, dent->d_name, NULL, 0, append))) 
			wlist_add (list, list->tail, ftmp);
	}
	closedir (dptr);
	free(dir);
	return;
}

int
write_mp3_list_file (wlist * list, char *filename)
{
	FILE *fp;
	flist *ftmp;

	
	
	if (!(fp = fopen (filename, "w")))
		return 1;
	fprintf (fp, "Playlist for mjs\n");
	for (ftmp = list->head; ftmp; ftmp = ftmp->next)
		fprintf (fp, "%s\n", ftmp->fullpath);
	fclose (fp);
	return 0;
}


void
read_mp3_list_file (wlist * list, const char *filename, int append)
{
	char *buf = NULL;
	char *file = NULL;
	FILE *fp;
	char *dir = NULL;
	char *playlistname = NULL;
	flist *ftmp = NULL;
	int length = 0, n = 0, lines = 0;
	int playlist = 0;

	errno = 0;
	if (!(fp = fopen (filename, "r")))
		return;

	buf = calloc (256, sizeof (char));
	while (!feof (fp)) {
		lines++;
		fgets (buf, 255, fp);
	}
	fclose (fp);
	errno = 0;
	if (!(fp = fopen (filename, "r")))
		return;

	fgets (buf, 255, fp);
	if (!strncmp ("Playlist for mjs", buf, 16))
		playlist = 1;

	if (playlist) {
		length = strrchr (filename, '/') - filename;
		playlistname = malloc (strlen (filename) - length - 4);
		strncpy (playlistname, filename + length + 1, strlen (filename) - length - 5);
		playlistname[strlen (filename) - length - 5] = '\0';
	} 

	if (append | !list->head)
		wlist_clear (list);

	if (append) {
		ftmp = calloc (1, sizeof (flist));
		ftmp->flags |= F_DIR;
		ftmp->filename = strdup ("../");
		ftmp->fullpath = getcwd (NULL, 0);
		if (playlist)
			ftmp->path = strdup (playlistname);
		else {
			char buf[21];
			snprintf((buf),20,"%d searchresults",(lines-2));
			ftmp->path = strdup (buf);
			list->flags |= F_VIRTUAL;
		}
		ftmp->relpath = strdup(ftmp->path);
		wlist_add (list, list->tail, ftmp);
	} else
		list->flags |= F_VIRTUAL;


	while (!feof (fp)) {
		n++;
		my_mvwaddstr (menubar->win, 0, (26 + (n * (50 / (float)lines))), colors[MENU_TEXT], "*");
		if ((n & 0x030)==0x000) {
			my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "|");
		} else
		if ((n & 0x030)==0x010) {
			my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "/");
		} else
		if ((n & 0x030)==0x020) {
			my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "-");
		} else
			my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "\\");
		update_panels ();
		doupdate ();
		
		if (!fgets (buf, 255, fp)) {
			// end-of-file reached or got zero characters
			fclose (fp);
			free (buf);
			if (playlistname)
				free (playlistname);
			return;
		}
		length = strrchr (buf, '/') - buf;
		buf[strlen (buf) - 1] = '\0';	// Get rid off trailing newline
		dir = malloc (length + 1);
		file = malloc (strlen (buf) - length);
		strncpy (dir, buf, length);
		dir[length] = '\0';
		strcpy (file, buf + length + 1);
//		if (*filename == '.')
//			continue;

		if ((ftmp = mp3_info (dir, file, playlistname, n, append)))
			wlist_add (list, list->tail, ftmp);

		free (file);
		free (dir);
	}
	fclose (fp);
	free (buf);
	if (playlistname)
		free (playlistname);
	return;
}

void
read_mp3_list_array (wlist * list, int argc, char *argv[])
{
	int n;
	flist *ftmp = NULL;
	wlist_clear(list);
	for (n = 1;n < argc ;n++) {
		char *dir, *file, *l;
		int length;
		
		if (l = strrchr (argv[n], '/')) {
			length = l - argv[n];
			dir = malloc (length + 1);
			strncpy (dir, argv[n], length);
			dir[length] = '\0';

			file = malloc (strlen (argv[n]) - length);
			strcpy (file, argv[n] + length + 1);

		} else {	/* no path given, so must be current working dir */
			char buf[256];
			getcwd(buf, 255);
			dir = strdup (buf);
			file = strdup (argv[n]);
			printf("%s %s\n", dir, file);
		}

		if ((ftmp = mp3_info (dir, file, NULL, n, L_NEW)))
			if (!(ftmp->flags & F_DIR))
				wlist_add (list, list->tail, ftmp);
		free(file);
		free(dir);
		}
	return;
}
/* sort directories first, then files. alphabetically of course. */
wlist *
sort_songs (wlist * sort)
{
	int i = 0, j;
	flist *ftmp = NULL, *newlist = NULL, **fsort;

	for (ftmp = sort->head; ftmp; ftmp = ftmp->next)
		i++;
	fsort = (flist **) calloc (i, sizeof (flist *));
	for (ftmp = sort->head, j = 0; ftmp; ftmp = ftmp->next, j++)
		fsort[j] = ftmp;

	qsort ((void *) fsort, i, sizeof (flist *), sort_mp3);
	sort->tail = newlist = fsort[0];
	newlist->next = NULL;

	for (j = 1; j < i; j++) {
		ftmp = fsort[j];
		ftmp->next = newlist;
		newlist->prev = ftmp;
		newlist = ftmp;
	}
	free (fsort);
	newlist->prev = NULL;
	sort->head = sort->top = sort->selected = newlist;
	return sort;
}

wlist *
sort_search (wlist * sort)
{
	int i = 0, j;
	flist *ftmp = NULL, *newlist = NULL, **fsort;

	
	
	for (ftmp = sort->head; ftmp; ftmp = ftmp->next)
		i++;
	fsort = (flist **) calloc (i, sizeof (flist *));
	for (ftmp = sort->head, j = 0; ftmp; ftmp = ftmp->next, j++)
		fsort[j] = ftmp;

	qsort ((void *) fsort, i, sizeof (flist *), sort_mp3_search);
	sort->tail = newlist = fsort[0];
	newlist->next = NULL;

	for (j = 1; j < i; j++) {
		ftmp = fsort[j];
		ftmp->next = newlist;
		newlist->prev = ftmp;
		newlist = ftmp;
	}
	free (fsort);
	newlist->prev = NULL;
	sort->head = sort->top = sort->selected = newlist;
	return sort;
}

static int
sort_mp3 (const void *a, const void *b)
{
	const flist *first = *(const flist **) a;
	const flist *second = *(const flist **) b;

	
	
	if (first->filename[0] == '.')
		return 1;
	if ((first->flags & (F_DIR|F_PLAYLIST)) && !(second->flags & (F_DIR|F_PLAYLIST)))
		return 1;
	else if (!(first->flags & (F_DIR|F_PLAYLIST)) && (second->flags & (F_DIR|F_PLAYLIST)))
		return -1;
	else
		return strcmp (second->filename, first->filename);
}

static int
sort_mp3_search (const void *a, const void *b)
{
	const flist *first = *(const flist **) a;
	const flist *second = *(const flist **) b;
	int result;

	
	
	if (first->filename[0] == '.')
		return 1;
	if (!(result = strcmp (second->path, first->path))) {
		if ((first->flags & F_DIR) && !(second->flags & F_DIR))
			return 1;
		else if (!(first->flags & F_DIR) && (second->flags & F_DIR))
			return -1;
		else
			return strcmp (second->filename, first->filename);
	}
	return result;
}

__inline__ int
check_file (flist * file)
{
	struct stat sb;
	
	if (!strncasecmp (file->fullpath, "http", 4))
		return 1;
	if (stat (file->fullpath, &sb) == -1)
		return 0;
	else
		return 1;
}

/* find the next valid entry in the search direction */

flist *
next_valid (wlist * list, flist * file, int c)
{
	flist *ftmp = NULL;
	
	
	
	if (!file)
		return NULL;
	switch (c) {
	case KEY_HOME:
	case KEY_DOWN:
	case KEY_NPAGE:
		while (!check_file (file)) {
			ftmp = file->next;
			wlist_del (list, file);
			file = next_valid (list, ftmp, c);
		}
		break;
	case KEY_END:
	case KEY_UP:
	case KEY_PPAGE:
		while (!check_file (file)) {
			ftmp = file->prev;
			wlist_del (list, file);
			file = next_valid (list, ftmp, c);
		}
		break;
	}
	return file;

}
