#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "mms.h"
#include "playlist.h"
#include "window.h"
#include "misc.h"
#include "files.h"
#include "mpgcontrol.h"
#include "id3.h"
#include "config.h"
#include "inputline.h"
#include "extern.h"

/* intialize the external variables */
Window *files, *info, *play, *active, *menubar, *old_active, *id3box;
Window *playback, *question;
Config *conf;
pid_t pid;
static struct sigaction handler;
int p_status = 0;
char version_str[128];

/* some internal functions */
static int	read_key(Window *);
static void	init_info(void);

int
main(int argc, char *argv[])
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

	if (!initscr())
		exit(1);
	curs_set(0);
	cbreak();
	noecho();
	start_color();
	nonl();
	init_ansi_pair();
#ifdef GPM_SUPPORT
	gpm_init();
#endif

	memset(version_str, 0, 128);
	snprintf(version_str, 128, "Matt's MP3 Selector v%s", VERSION);

/* malloc() for the windows and set up our initial callbacks ... */

	info = calloc(1, sizeof(Window));
	play = calloc(1, sizeof(Window));
	playback = calloc(1, sizeof(Window));
	menubar = calloc(1, sizeof(Window));
	id3box = calloc(1, sizeof(Window));
	active = files = calloc(1, sizeof(Window));

/* reading the config must take place AFTER initscr() and friends */

	info->update = update_info;
	info->activate = active_win;
	info->deactivate = inactive_win;
	info->prev = files;
	info->next = play;
	info->input = read_key;
	info->flags |= W_RDONLY;

/* in theory, this window should NEVER be active, but just in case ... */
	playback->update = dummy_update; /* generic dummy update */
	playback->activate = active_win;
	playback->deactivate = inactive_win;
	playback->prev = NULL; /* can't tab out of this! */
	playback->next = NULL;
	playback->input = read_key;
	playback->flags |= W_RDONLY;

	id3box->update = update_edit;
	id3box->activate = active_win;
	id3box->deactivate = inactive_edit;
	id3box->prev = NULL; /* can't tab out of this! */
	id3box->next = NULL;
	id3box->input = read_key;

	play->update = show_list;
	play->activate = active_win;
	play->deactivate = inactive_win;
	play->next = files;
	play->input = read_key;
	play->flags |= W_LIST;

	files->update = show_list;
	files->activate = active_win;
	files->deactivate = inactive_win;
	files->prev = play;
	files->input = read_key;
	files->flags |= W_LIST | W_RDONLY;

	/* now we have initialized most of the window stuff, read our config */
	conf = calloc(1, sizeof(Config));
	strncpy(conf->mpgpath, MPGPATH, 255);
	read_config(conf);

	/* these depend on config file settings */
	files->next = (conf->c_flags & C_SKIPINFO) ? play : info;
	play->prev = (conf->c_flags & C_SKIPINFO) ? files : info;

	/* check window settings for sanity -- not perfect yet :) */
	/* these are needed so the mouse doesn't get confused     */

	if (files->height == 0) files->height = LINES - files->y;
	if (files->width < 4) files->width = COLS - files->x;
	if (play->height == 0) play->height = LINES - play->y;
	if (play->width < 4) play->width = COLS - play->x;
	if (info->height < 6) info->height = 6;
	if (info->width < 4) info->width = COLS - info->x;
	if (menubar->height == 0) menubar->height = 1;
	if (menubar->width == 0) menubar->width = COLS - menubar->x;
	if (playback->height < 3) playback->height = 3;
	if (playback->width == 0) playback->width = COLS - playback->x;
	if (id3box->height < 7) id3box->height = 7;
	if (id3box->width < 40) id3box->width = 50;

	active->win = files->win = newwin(files->height, files->width, files->y, files->x);
	info->win = newwin(info->height, info->width, info->y, info->x);
	play->win = newwin(play->height, play->width, play->y, play->x);
	id3box->win = newwin(id3box->height, id3box->width, id3box->y, id3box->x);
	menubar->win = newwin(menubar->height, menubar->width, menubar->y, menubar->x);
	playback->win = newwin(playback->height, playback->width, playback->y, playback->x);
	menubar->input = read_key;

	if (!files->win || !info->win || !play->win || !menubar->win || !id3box->win)
		bailout(0);

	/* create the panels in this order to create the initial stack */
	menubar->panel = new_panel(menubar->win);
	info->panel = new_panel(info->win);
	play->panel = new_panel(play->win);
	files->panel = new_panel(files->win);
	id3box->panel = new_panel(id3box->win);
	playback->panel = new_panel(playback->win);

	hide_panel(id3box->panel);

	keypad(files->win, TRUE);
	keypad(info->win, TRUE);
	keypad(play->win, TRUE);
	keypad(menubar->win, TRUE);
	keypad(id3box->win, TRUE);

	wbkgd(files->win, colors[FILE_BACK]);
	wbkgd(info->win, colors[INFO_BACK]);
	wbkgd(play->win, colors[PLAY_BACK]);
	wbkgd(menubar->win, colors[MENU_BACK]);
	wbkgd(id3box->win, colors[EDIT_BACK]);
	wbkgd(playback->win, colors[INFO_BACK]);
	my_mvwaddstr(menubar->win, 0, 28, colors[MENU_TEXT], version_str);
	init_info();

	play->deactivate(play);
	info->deactivate(info);
	active->activate(active);
	playback->deactivate(playback);

	files->contents.list = mp3list = (wlist *)calloc(1, sizeof(wlist));
	mp3list->head = read_mp3_list(mp3list);
	sort_songs(mp3list);
	play->contents.list = (wlist *)calloc(1, sizeof(wlist));
	info->contents.play = NULL;

	files->update(files);
	update_info(files);
	if (*conf->dfl_plist) {
		play->contents.list = read_playlist(play->contents.list, conf->dfl_plist);
		if (play->contents.list->head) {
			active->deactivate(active);
			active->update(active);
			play->activate((active = play));
			play->update(play);
		}
	}
	doupdate();

	start_mpg_child();
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		add_player_descriptor(&fds);
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
			else 
				check_player_output(&fds);
		}
	}
	bailout(0);
}

void
bailout(int sig)
{
	/* Lets get the hell out of here! */
	wclear(stdscr);
	refresh();
	endwin();
#ifdef GPM_SUPPORT
	gpm_close();
#endif
	if (pid > 0) {
		pid_t pgrp = getpgid(pid);
		fprintf(stdout, "Cleaning up ...\n");
		fflush(stdout);
		handler.sa_handler = SIG_DFL;
		handler.sa_flags = 0;
		sigaction(SIGCHLD, &handler, NULL);
		send_cmd(QUIT);
		/* kill the entire process group, for the buffering child */
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)         
		killpg(pgrp, SIGTERM);
#else
		kill(-pgrp, SIGTERM); 
#endif /* *BSD */
		waitpid(pid, NULL, 0); /* be safe and avoid a zombie */
	}
	exit(0);
}

static int
read_key(Window *window)
{
	int c, alt = 0;
	Input *inputline = window->inputline;

	c = WGETCH(window->win);
#ifdef GPM_SUPPORT
	if (gpm_hflag)
		return 0;
#endif
	if (c == 27) {
		alt = 1;
		c = WGETCH(window->win);
	}

	if (inputline)
		return inputline->parse(inputline, c, alt);
	
	switch (c) {
		case '\t':
			if (active->next)
				change_active(active->next);
			break;
		case KEY_BTAB:
			if (active->prev)
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
			if (window == info)
				edit_tag(window->contents.play);
			else if (!(window->contents.list->selected->flags & F_DIR))
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
				playlist->selected = delete_file(active, playlist->selected);
				info->update(play);
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
				clear_play_info();
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
				active->update(active);
				play->activate((active = play));
				play->update(play);
				info->update(play);
			} else if (active == files) {
				wlist *mp3list = active->contents.list;
				free_list(mp3list->head);
				memset(mp3list, 0, sizeof(wlist));
				mp3list->head = read_mp3_list(mp3list);
				if (mp3list->head)
					sort_songs(mp3list);
				files->update(files);
			}
			doupdate();
			break;
		case '+':
			if (p_status)
				send_cmd(JUMP, 1000);
			break;
		case '-':
			if (p_status)
				send_cmd(JUMP, -1000);
			break;
		case 'l':
		case 'L':
		case KEY_F(1):
			old_active = active;
			active = menubar;
			wbkgd(menubar->win, 0);
			menubar->inputline = inputline = (Input *)calloc(1, sizeof(Input));
			inputline->win = menubar->win;
			inputline->panel = menubar->panel;
			inputline->x = inputline->y = 0;
			strncpy(inputline->prompt, "Path to playlist:", 39);
			inputline->plen = strlen(inputline->prompt);
			inputline->flen = 60;
			inputline->anchor = inputline->buf;
			inputline->parse = do_inputline;
			inputline->update = update_menu;
			inputline->finish = do_read_playlist;
			inputline->complete = filename_complete;
			inputline->pos = 1; /* start out one "space" into it */
			inputline->fpos = 1;
			curs_set(1);
			update_menu(inputline);
			doupdate();
			break;
		case KEY_F(2):
			old_active = active;
			active = menubar;
			wbkgd(menubar->win, 0);
			menubar->inputline = inputline = (Input *)calloc(1, sizeof(Input));
			inputline->win = menubar->win;
			inputline->panel = menubar->panel;
			inputline->x = inputline->y = 0;
			strncpy(inputline->prompt, "Save Path:", 39);
			inputline->plen = strlen(inputline->prompt);
			inputline->flen = 60;
			inputline->anchor = inputline->buf;
			inputline->parse = do_inputline;
			inputline->update = update_menu;
			inputline->finish = do_save_playlist;
			inputline->complete = filename_complete;
			inputline->pos = 1; /* start out one "space" into it */
			inputline->fpos = 1;
			curs_set(1);
			update_menu(inputline);
			doupdate();
			break;
		default:
			break;
	}
	return c;
}

void
process_return(wlist *mp3list)
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
			if (mp3list->head)
				sort_songs(mp3list);
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
							mp3list->head->colors = colors[SELECTED];
							mp3list->selected = ftmp;
							ftmp->flags |= F_SELECTED;
							ftmp->colors = colors[SELECTED];
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
			if (conf->c_flags & C_FADVANCE)
				if (info->update(move_selector(files, KEY_DOWN)))
					files->update(files);
			play->update(play);
		}
		doupdate();
	} else if (active == play)
		jump_to_song(active->contents.list->selected);
}

void
unsuspend(int sig)
{
	wrefresh(curscr);
	curs_set(0);
}


int
update_menu(Input *inputline)
{
	wmove(inputline->win, inputline->y, inputline->x);
	my_wnclear(inputline->win, inputline->flen);
	update_anchor(inputline);
	my_wnprintw(inputline->win, colors[MENU_TEXT], inputline->flen + inputline->plen, "%s", inputline->prompt);
	my_wnprintw(inputline->win, inputline->y, inputline->flen + inputline->plen, " %s", inputline->anchor);
	wmove(inputline->win, inputline->y, inputline->fpos+inputline->plen);
	update_panels();
	doupdate();
	return 1;
}

void
update_status(void)
{
	extern int p_status;
	
	if (!play->contents.list)
		return;
	
	if (p_status == 0) {
		clear_play_info();
		play_next_song();
		doupdate();
	}
}

void
show_playinfo(mpgreturn *message)
{
	int minleft, minused;
	double secleft, secused;
	
	minleft = (int)message->remaining / 60;
	secleft = message->remaining - minleft*60;
	minused = (int)message->elapsed / 60;
	secused = message->elapsed - minused*60;
	my_mvwnprintw2(playback->win, 1, 7, colors[UNSELECTED], playback->width-9,
		"Time: %02d:%05.2f/%02d:%05.2f   Frames: (%d/%d)    ",
		minused, secused, minleft, secleft, message->played, message->left);
	if (active->inputline) {
		Input *inputline = active->inputline;
		wmove(inputline->win, inputline->y, inputline->fpos+inputline->plen);
	}
	top_panel(playback->panel);
	update_panels();
	doupdate();
}

__inline__ void
clear_play_info(void)
{
	my_mvwnclear(playback->win, 1, 7, playback->width-9);
	update_panels();
}

int
dummy_update(Window *window)
{
	return 1;
}

static void init_info(void)
{
	WINDOW *win = info->win;

	my_mvwaddstr(win, 1, 2, colors[UNSELECTED], "Filename:");
	my_mvwaddstr(win, 2, 4, colors[UNSELECTED], "Artist:");
	my_mvwaddstr(win, 3, 5, colors[UNSELECTED], "Title:");
	my_mvwaddstr(win, 4, 4, colors[UNSELECTED], "Length:        Genre:                Bitrate:");
	update_panels();
}

