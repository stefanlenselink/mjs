#include "defs.h"
#include "mms.h"
#include "colors.h"
#include "struct.h"
#include "proto.h"
#include "extern.h"

/* intialize the external variables */
Window *files, *info, *play, *active, *menubar;
pid_t pid;
Input *inputline = NULL;
struct sigaction handler;
int p_status = 0;
char version_str[] = "Matt's MP3 Selector v0.84";

/* inpipe and outpipe connect to stdin and stdout of child */
int inpipe[2], outpipe[2];

/* some internal functions */
static int read_key (Window *);

int
main (int argc, char *argv[])
{
	wlist *mp3list = NULL;
	fd_set fds;
	


	srand(time(NULL));
	memset(&handler, 0, sizeof(struct sigaction));
	
	handler.sa_handler = (SIGHANDLER) bailout;
	sigaction(SIGINT, &handler, NULL);
	handler.sa_handler = (SIGHANDLER) unsuspend;
	handler.sa_flags = SA_RESTART;
	sigaction(SIGCONT, &handler, NULL);
	handler.sa_handler = (SIGHANDLER) restart_mpg_child;
	handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &handler, NULL);

	initscr();
	curs_set(0);
	cbreak();
	noecho();
	start_color();
	nonl();
	init_ansi_pair();
#ifdef GPM_SUPPORT
	gpm_init();
#endif



/* malloc() for the windows and set up our initial callbacks ... */

	info = calloc(1, sizeof(Window));
	play = calloc(1, sizeof(Window));
	menubar = calloc(1, sizeof(Window));
	active = files = calloc(1, sizeof(Window));
	active->flags |= W_ACTIVE;

/* reading the config must take place AFTER initscr() and friends */

	memset(mpgpath, 0, sizeof(mpgpath));
	strncpy(mpgpath, MPGPATH, sizeof(mpgpath)-1);
	memset(dfl_plist, 0, sizeof(dfl_plist));
	read_config();

	info->update = update_info;
	info->activate = active_win;
	info->deactivate = inactive_win;
	info->prev = files;
	info->next = play;
	info->input = read_key;
	info->title = "MP3 Info";
	info->flags |= W_RDONLY;
	
	play->update = show_list;
	play->activate = active_win;
	play->deactivate = inactive_win;
	play->prev = skip_info_box ? files : info;
	play->next = files;
	play->title = "Playlist";
	play->input = read_key;
	play->flags |= W_LIST;

	files->update = show_list;
	files->activate = active_win;
	files->deactivate = inactive_win;
	files->prev = play;
	files->next = skip_info_box ? play : info;
	files->title = "MP3  Files";
	files->input = read_key;
	files->flags |= W_LIST | W_RDONLY;

	/* check window settings for sanity -- not perfect yet :) */
	/* these are needed so the mouse doesn't get confused     */

	if (files->height == 0) files->height = LINES - files->y;
	if (files->width < 4) files->width = COLS - files->x;
	if (play->height == 0) play->height = LINES - play->y;
	if (play->width < 4) play->width = COLS - play->x;
	if (info->height < 8) info->height = LINES - info->y;
	if (info->width < 4) info->width = COLS - info->x;
	if (menubar->height == 0) menubar->height = LINES - menubar->y;
	if (menubar->width == 0) menubar->width = COLS - menubar->x;

	active->win = files->win = newwin(files->height, files->width, files->y, files->x);
	info->win = newwin(info->height, info->width, info->y, info->x);
	play->win = newwin(play->height, play->width, play->y, play->x);
	menubar->win = newwin(menubar->height, menubar->width, menubar->y, menubar->x);

	if (!files->win || !info->win || !play->win || !menubar->win)
		bailout(0);

	/* create the panels in this order to create the initial stack */
	menubar->panel = new_panel(menubar->win);
	info->panel = new_panel(info->win);
	play->panel = new_panel(play->win);
	files->panel = new_panel(files->win);

	keypad(files->win, TRUE);
	keypad(info->win, TRUE);
	keypad(play->win, TRUE);
	keypad(menubar->win, TRUE);

	wbkgd(files->win, colors[FILE_BACK]);
	wbkgd(info->win, colors[INFO_BACK]);
	wbkgd(play->win, colors[PLAY_BACK]);
	wbkgd(menubar->win, colors[MENU_BACK]);
	my_mvwaddstr(menubar->win, 0, 28, colors[MENU_TEXT], version_str);
	update_panels();

	play->deactivate(play);
	info->deactivate(info);
	active->activate(active);
	
	files->contents.list = mp3list = (wlist *)calloc(1, sizeof(wlist));
	mp3list->head = read_mp3_list(mp3list);
	read_songs(mp3list->head);
	sort_songs(mp3list);
	play->contents.list = (wlist *)calloc(1, sizeof(wlist));
	info->contents.play = NULL;

	files->update(files);
	update_info(files);
	if (*dfl_plist) {
		play->contents.list = read_playlist(play->contents.list, dfl_plist);
		active->deactivate(active);
		active->flags &= ~W_ACTIVE;
		active->update(active);
		play->activate((active = play));
		active->flags |= W_ACTIVE;
		play->update(play);
	}
	doupdate();

	if (pipe(inpipe) || pipe(outpipe))
		bailout(0);
	fcntl(outpipe[1], F_SETFD, O_NONBLOCK);
	start_mpg_child();
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(outpipe[0], &fds);
#ifdef GPM_SUPPORT
		if (gpm_fd > 0)
			FD_SET(gpm_fd, &fds);
#endif
		if (select(FD_SETSIZE, &fds, NULL, NULL, NULL) > 0) {
			if (FD_ISSET(0, &fds)
#ifdef GPM_SUPPORT
			|| (gpm_fd > 0 && FD_ISSET(gpm_fd, &fds))
#endif
			)
				active->input(active);
			else if FD_ISSET(outpipe[0], &fds)
				mpg_output(outpipe[0]);
		}
	}
	bailout(0);
}

void
bailout (int sig)
{
	/* Lets get the hell out of here! */
	wclear(stdscr);
	refresh();
	endwin();
	send_cmd(inpipe[1], QUIT);
#ifdef GPM_SUPPORT
	gpm_close();
#endif
	exit(0);
}

static int
read_key (Window *window)
{
	int c, alt = 0;

	if (inputline)
		return inputline->parse(inputline);
	
	c = WGETCH(window->win);
#ifdef GPM_SUPPORT
	if (gpm_hflag)
		return 0;
#endif
	if (c == 27) {
		alt = 1;
		c = WGETCH(window->win);
	}
	switch (c) {
		case '\t':
			change_active(active->next);
			break;
		case KEY_BTAB:
			change_active(active->prev);
			break;
		case KEY_HOME:
		case KEY_END:
		case KEY_DOWN:
		case KEY_UP:
		case KEY_PPAGE:
		case KEY_NPAGE:
			if ((window->flags & W_LIST) && info->update(move_selector(window, c))) {
				window->update(window);
				doupdate();
			}
			break;
		case 'e':
		case 'E':
			if (!p_status && window != info && !(window->contents.list->selected->flags & F_DIR))
				edit_tag(window->contents.list->selected);
			break;
		case KEY_RIGHT:
		case 'f':
		case 'F':
			if (p_status)
				jump_forward(play->contents.list);
				doupdate();
			break;
		case KEY_LEFT:
		case 'b':
		case 'B':
			if (p_status)
				jump_backward(play->contents.list);
				doupdate();
			break;
		case 'q':
		case 'Q':
			bailout(0);
		case KEY_ENTER:
		case '\n':
		case '\r':
			process_return(window->contents.list);
			break;
		case KEY_DC:
			if (!(active->flags & W_RDONLY)) {
				wlist *playlist = active->contents.list;
				playlist->selected = delete_selected(playlist, playlist->selected);
				active->update(active);
				doupdate();
			}
			break;
		case KEY_REFRESH:
			wrefresh(curscr);
			break;
		case 's':
		case 'S':
			if (p_status)
				stop_player(play->contents.list);
				clear_play_info(info->win);
				doupdate();
			break;
		case 'p':
		case 'P':
			if (p_status > 0) {
				pause_player(play->contents.list);
				play->update(play);
				doupdate();
			}
			break;
		case 'r':
		case 'R':
			if (active == play || active == info) {
				randomize_list(play->contents.list);
				active->deactivate(active);
				active->flags &= ~W_ACTIVE;
				active->update(active);
				play->activate((active = play));
				active->flags |= W_ACTIVE;
				play->update(play);
				info->update(play);
			} else if (active == files) {
				wlist *mp3list = active->contents.list;
				free_list(mp3list->head);
				memset(mp3list, 0, sizeof(wlist));
				mp3list->head = read_mp3_list(mp3list);
				if (mp3list->head) {
					read_songs(mp3list->head);
					sort_songs(mp3list);
				}
				files->update(files);
			}
			doupdate();
			break;
		case '+':
			if (p_status)
				send_cmd(inpipe[1], JUMP, 1000);
			break;
		case '-':
			if (p_status)
				send_cmd(inpipe[1], JUMP, -1000);
			break;
		case 'l':
		case 'L':
		case KEY_F(1):
			curs_set(1);
			wbkgd(menubar->win, 0);
			inputline = (Input *)calloc(1, sizeof(Input));
			inputline->win = menubar->win;
			inputline->panel = menubar->panel;
			strncpy(inputline->prompt, "Path to playlist:", 39);
			inputline->plen = strlen(inputline->prompt);
			inputline->flen = 60;
			inputline->anchor = inputline->buf;
			inputline->parse = do_inputline;
			inputline->update = update_menu;
			inputline->finish = do_read_playlist;
			inputline->pos = 1; /* start out one "space" into it */
			inputline->fpos = 1;
			update_menu(inputline);
			doupdate();
			break;
		case KEY_F(2):
			curs_set(1);
			wbkgd(menubar->win, 0);
			inputline = (Input *)calloc(1, sizeof(Input));
			inputline->win = menubar->win;
			inputline->panel = menubar->panel;
			strncpy(inputline->prompt, "Save Path:", 39);
			inputline->plen = strlen(inputline->prompt);
			inputline->flen = 60;
			inputline->anchor = inputline->buf;
			inputline->parse = do_inputline;
			inputline->update = update_menu;
			inputline->finish = do_save_playlist;
			inputline->pos = 1; /* start out one "space" into it */
			inputline->fpos = 1;
			update_menu(inputline);
			doupdate();
			break;
		default:
			break;
	}
	return c;
}

flist *
read_mp3_list (wlist *list)
{
	char *dir = NULL;
	DIR *dptr = NULL;
	struct dirent *dent;
	struct stat st;
	flist *ftmp = NULL, *mp3list = list->head, *tmp;
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
			ftmp->filename = malloc(dent->d_reclen+2);
			strcpy(ftmp->filename, dent->d_name);
			strcat(ftmp->filename, "/");
			ftmp->next = mp3list;
			if (mp3list)
				mp3list->prev = ftmp;
			mp3list = ftmp;
			length++;
		} else if (S_ISREG(st.st_mode)) {
			if (strncasecmp(".mp3", strchr(dent->d_name, '\0')-4, 4))
				continue;
			if (!(tmp = mp3_info(dent->d_name, st.st_size)))
				continue;
			ftmp = tmp;
			ftmp->filename = strdup(dent->d_name);
			ftmp->path = strdup(dir);
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
show_list (Window *window)
{
	int x = window->width-4, y = window->height-1, i, tmp = 0;
	WINDOW *win = window->win;
	flist *ftmp;

	if (!window->contents.list)
		return 0;
	ftmp = window->contents.list->top;
	
	if (p_status == 2)
		tmp = A_BLINK;
	for (i = 1; i < y; i++) {
		if (ftmp && *ftmp->filename) {
			if ((window->flags & W_ACTIVE) && (ftmp->flags & F_SELECTED)) {
				if (ftmp->flags & F_PLAY)
					my_mvwnaddstr(win, i, 2, colors[SEL_PLAYING] | tmp, x, ftmp->filename);
				else
					my_mvwnaddstr(win, i, 2, colors[SELECTED], x, ftmp->filename);
			} else if (ftmp->flags & F_PLAY)
				my_mvwnaddstr(win, i, 2, colors[PLAYING] | tmp, x, ftmp->filename);
			else
				my_mvwnaddstr(win, i, 2, colors[UNSELECTED], x, ftmp->filename);
			ftmp = ftmp->next;
		} else /* blank the line */
			my_mvwnaddstr(win, i, 2, colors[FILE_BACK], x, "");
	}
	if (active->flags & W_LIST)
		do_scrollbar(active);
	update_panels();
	return 1;
}

Window *
move_selector (Window *window, int c)
{
	flist *ftmp, *list;
	wlist *wlist = window->contents.list;
	int i, j, maxx, maxy, length;
	
	if (!wlist)
		return NULL;
	
	getmaxyx(window->win, maxy, maxx);
	length = maxy - 2;
	maxy -= 3;

	/* check these two cases here because we can avoid the for() loop below */
	if (c == KEY_HOME) {
		wlist->selected->flags &= ~F_SELECTED;
		wlist->top = wlist->head;
		wlist->selected = wlist->head;
		wlist->selected->flags |= F_SELECTED;
		wlist->where = 1;
		return window;
	} else if (c == KEY_END) {
		wlist->selected->flags &= ~F_SELECTED;
		wlist->selected = wlist->tail;
		wlist->selected->flags |= F_SELECTED;
		wlist->where = wlist->length;
		if (length < wlist->length) {
			for (i = 1, ftmp = wlist->tail; ftmp && (i < length); i++)
				ftmp = ftmp->prev;
			wlist->top = ftmp;
		} else
			wlist->top = wlist->head;
		return window;
	}
	ftmp = list = wlist->top;
	/* count the distance from the top to selected... only a few loops */
	for (i = 0; ftmp && !(ftmp->flags & F_SELECTED) && (i < maxy); i++)
		ftmp = ftmp->next;
	if (!ftmp)
		return NULL;
	switch (c) {
		case KEY_DOWN:
			if (ftmp->next) {
				ftmp->flags &= ~F_SELECTED;
				ftmp = ftmp->next;
				ftmp->flags |= F_SELECTED;
				wlist->selected = ftmp;
				wlist->where++;
				if (i == maxy)
					wlist->top = list->next;
				return window;
			}
			break;
		case KEY_UP:
			if (ftmp->prev) {
				ftmp->flags &= ~F_SELECTED;
				ftmp = ftmp->prev;
				ftmp->flags |= F_SELECTED;
				wlist->selected = ftmp;
				wlist->where--;
				if (i == 0)
					wlist->top = list->prev;
				return window;
			}
			break;
		case KEY_NPAGE:
			ftmp->flags &= ~F_SELECTED;
			for (j = 0; ftmp->next && j < length; j++) {
				ftmp = ftmp->next;
				wlist->where++;
			}
			if (i+j > maxy)
				for (i = 0; i < j; i++)
					list = list->next;
			wlist->top = list;
			wlist->selected = ftmp;
			ftmp->flags |= F_SELECTED;
			return window;
		case KEY_PPAGE:
			ftmp->flags &= ~F_SELECTED;
			for (j = 0; ftmp->prev && j < length; j++) {
				wlist->where--;
				ftmp = ftmp->prev;
				if (list->prev)
					list = list->prev;
			}
			wlist->top = list;
			wlist->selected = ftmp;
			ftmp->flags |= F_SELECTED;
			return window;
		default:
			break;
	}
	return NULL;
}

int
update_info (Window *window)
{
	WINDOW *win = info->win;
	int i = 0;
	flist *file;

	if (!window || !window->contents.list)
		return 0;

	if (window->flags & W_LIST)
		file = window->contents.list->selected;
	else
		file = window->contents.play;
	if (!file)
		return 0;
	my_mvwnprintw(win, 1, 1, colors[UNSELECTED], info->width-4, " Filename: %n%-*s", &i, info->width-4-i,
		(file->flags & F_DIR) ? "(Directory)" : file->filename);
	my_mvwprintw(win, 2, 1, colors[UNSELECTED], "   Artist: %n%-*s", &i, info->width-4-i, file->artist ? :"");
	my_mvwprintw(win, 3, 1, colors[UNSELECTED], "    Title: %n%-*s", &i, info->width-4-i, file->title ? :"");
	mvwaddstr(win, 4, 12, "                ");
	my_mvwprintw(win, 4, 4, colors[UNSELECTED], "Length: ");
	if (!(file->flags & F_DIR)) {
		my_mvwprintw(win, 4, 12, colors[UNSELECTED], "%lu:%02d", file->length/60, file->length % 60);
		my_mvwprintw(win, 5, 3, colors[UNSELECTED], "Bitrate: %-3d\t\tFrequency: %d", file->bitrate, file->frequency);
	}
	else
		my_mvwprintw(win, 5, 3, colors[UNSELECTED], "Bitrate:    \t\tFrequency:\t\t");
	update_panels();
	return 1;
}

int
start_mpg_child(void)
{
	int i;

	handler.sa_handler = SIG_DFL;
	handler.sa_flags = 0;
	sigaction(SIGCHLD, &handler, NULL);

	errno = 0;
	switch (pid = fork()) {
		case -1:
			my_mvwprintw(menubar->win, 0, 0, colors[MENU_TEXT], "Error: %s", strerror(errno));
			break;
		case 0:
			dup2(inpipe[0], 0);
			dup2(outpipe[1], 1);
			inpipe[1] = outpipe[0] = -1;
			for (i = 2; i < 256; i++)
				close(i);
			errno = 0;
			execlp(mpgpath, "mpg123", "-R", "-", (char *)NULL);
			if (errno)
				exit(0);
		default:
			handler.sa_handler = (SIGHANDLER) restart_mpg_child;
			handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
			sigaction(SIGCHLD, &handler, NULL);
			break;
	}
	return pid;
}

void
restart_mpg_child(void)
{
	wlist *win;

	/* we will use this to clean up on a SIGCHLD */
	if (pid)
		waitpid(pid, NULL, 0);
	pid = 0;
	
	start_mpg_child();
	
	win = play->contents.list;
	if (win && win->playing)
		play_next_song();
}

/* sort directories first, then files. alphabetically of course. */
wlist *
sort_songs (wlist *sort)
{
	int i = 0, j;
	flist *ftmp = NULL, *newlist = NULL, **fsort;
	
	for (ftmp = sort->head; ftmp; ftmp = ftmp->next)
		i++; 
	fsort = (flist **) calloc (i, sizeof(flist *));
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

int
sort_mp3 (const void *a, const void *b)
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

void
process_return (wlist *mp3list)
{
	if (!mp3list)
		return;
	if (active == files) {
		if (mp3list->selected->flags & F_DIR) {
			char *prevpwd = NULL;
			if (!strcmp("../", mp3list->selected->filename))
				prevpwd = getcwd(NULL, 0);
			chdir(mp3list->selected->filename);
			free_list(mp3list->head);
			memset(mp3list, 0, sizeof(wlist));
			mp3list->head = read_mp3_list(mp3list);
			if (mp3list->head) {
				read_songs(mp3list->head);
				sort_songs(mp3list);
			}
			/* egads this is ugly */
			if (prevpwd) {
/* -- gone for now until i rework it
				flist *ftmp;
				char *p = NULL;
				if ((p = strrchr(prevpwd, '/'))) {
					int i = strlen(p);
					*p++ = '\0';
					memmove(prevpwd, p, i+1);
					strcat(prevpwd, "/");
					for (i = 0, ftmp = mp3list->head; ftmp; ftmp = ftmp->next, i++) {
						if (!strcmp(ftmp->filename, prevpwd)) {
							mp3list->head->flags &= ~F_SELECTED;
							mp3list->selected = ftmp;
							ftmp->flags |= F_SELECTED;
							mp3list->where += i;
							break;
						}
					}
				}
*/
				free(prevpwd);
			}
			files->update(files);
		} else {
			play->contents.list = add_to_playlist(play->contents.list, mp3list->selected);
			play->update(play);
		}
		doupdate();
	} else if (active == play)
		jump_to_song(active->contents.list->selected);
}

void
unsuspend (int sig)
{
	wrefresh(curscr);
	curs_set(0);
}


int
update_menu (Input *inputline)
{
	wmove(menubar->win, 0, 0);
	wclrtoeol(menubar->win);
	update_anchor(inputline);
	my_wnprintw(menubar->win, colors[MENU_TEXT], menubar->width, "%s", inputline->prompt);
	my_wnprintw(menubar->win, 0, menubar->width, " %s", inputline->anchor);
	wmove(menubar->win, 0, inputline->fpos+inputline->plen);
	update_panels();
	doupdate();
	return 1;
}

int
mpg_output (int fd)
{
	mpgreturn message;
	
	memset(&message, 0, sizeof(mpgreturn));
	if (read_cmd(fd, &message)) {
		show_playinfo(&message);
		return 1;
	}
	update_status();
	return 0;
}

void
show_playinfo (mpgreturn *message)
{
	int minleft, minused;
	double secleft, secused;
	
	minleft = (int)message->remaining / 60;
	secleft = message->remaining - minleft*60;
	minused = (int)message->elapsed / 60;
	secused = message->elapsed - minused*60;
	my_mvwnprintw2(info->win, info->height-2, 6, colors[UNSELECTED], info->width-8,
		"Time: %02d:%05.2f/%02d:%05.2f   Frames: (%d/%d)    ",
		minused, secused, minleft, secleft, message->played, message->left);
	update_panels();
	doupdate();
}

void
update_status (void)
{
	extern int p_status;
	
	if (!play->contents.list)
		return;
	
	if (p_status == 0) {
		clear_play_info(info->win);
		play_next_song();
		doupdate();
	}
}

__inline__ void
clear_play_info (WINDOW *win)
{
	my_mvwnprintw2(win, info->height-2, 2, colors[UNSELECTED], info->width-4,
		"  ");
	update_panels();
}

void change_active(Window *new)
{
	active->deactivate(active);
	active->flags &= ~W_ACTIVE;
	active->update(active);
	active = new;
	active->activate(active);
	active->flags |= W_ACTIVE;
	active->update(active);
	info->update(active);
	doupdate();
}
