#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "mjs.h"
#include "playlist.h"
#include "window.h"
#include "misc.h"
#include "files.h"
#include "mpgcontrol.h"
#include "config.h"
#include "inputline.h"
#include "extern.h"
#include "unistd.h"

/* intialize the external variables */
Window *files, *info, *play, *active, *menubar, *old_active;
Window *playback, *question;
Config *conf;
pid_t pid;
static struct sigaction handler;
int p_status = STOPPED;
char *previous_selected;		// previous selected number
char typed_letters[10] = "\0";	// letters previously typed when jumping
int typed_letters_timeout = 0;		// timeout for previously typed letters

/* some internal functions */
static int	read_key(Window *);
static void	init_info(void);

int
do_save(Input* input)
{
	char *s = malloc(strlen(input->buf)+26);
	active=old_active;
        sprintf(s,"%s/%s.mjs", conf->playlistpath, input->buf);
	write_mp3_list_file(play->contents.list,s);
	free(input);
	menubar->activate(menubar);
	menubar->inputline = NULL;
	update_panels();
	doupdate();
	return 1;
}

int
do_search(Input* input)
{
	pid_t childpid;
	wlist *mp3list = files->contents.list;
	menubar->deactivate(menubar);
	active=old_active;
	my_mvwaddstr(menubar->win, 0, 2, colors[MENU_TEXT], "Busy searching.....  [                                                   ]");
	update_panels();
	doupdate();
	if (!((*input->buf==' ')||(*input->buf== '\0'))){
		handler.sa_handler = SIG_DFL;
		handler.sa_flags = SA_ONESHOT;
		sigaction(SIGCHLD, &handler, NULL);
		errno = 0;
		if (!(childpid = fork())){
			errno = 0;
			execlp("findmp3", "findmp3", input->buf, conf->resultsfile, (char *)NULL);
			exit(3);
			}
		if (errno)
			exit(3);

		waitpid(childpid,NULL,0);		

		handler.sa_handler = (SIGHANDLER) unsuspend;
		handler.sa_flags = SA_RESTART;
		sigaction(SIGCONT, &handler, NULL);
		handler.sa_handler = (SIGHANDLER) restart_mpg_child;
		handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
		sigaction(SIGCHLD, &handler, NULL);
		free_list(mp3list->head);
		free(mp3list);
		mp3list = read_mp3_list_file(NULL, conf->resultsfile);
		if (mp3list->head)
			sort_search(mp3list);
		files->update(files);
		}
	free(input);
	menubar->activate(menubar);
	menubar->inputline = NULL;
	update_panels();
	doupdate();
	return 1;
}


int
main(int argc, char *argv[])
{
	wlist *mp3list = NULL;
	struct timeval wait1000 = {0, 1000000};
	fd_set fds;
	
	srand(time(NULL));
	previous_selected = strdup("\0");
	
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
	leaveok(stdscr,TRUE);
	cbreak();
	noecho();
//	use_default_colors();
	start_color();
	nonl();
	init_ansi_pair();
	if (argc>1)
		bailout(5);
/* malloc() for the windows and set up our initial callbacks ... */

	info = calloc(1, sizeof(Window));
	play = calloc(1, sizeof(Window));
	playback = calloc(1, sizeof(Window));
	menubar = calloc(1, sizeof(Window));
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

	play->update = show_list;
	play->activate = active_win;
	play->deactivate = inactive_win;
	play->prev = files;
	play->next = files;
	play->input = read_key;
	play->flags |= W_LIST;

	files->update = show_list;
	files->activate = active_win;
	files->deactivate = inactive_win;
	files->prev = play;
	files->next = play;
	files->input = read_key;
	files->flags |= W_LIST | W_RDONLY;

	menubar->activate = std_menubar;
	menubar->deactivate = clear_menubar;

	/* now we have initialized most of the window stuff, read our config */
	conf = calloc(1, sizeof(Config));
	strncpy(conf->mpgpath, MPGPATH, 255);
	read_config(conf);


	/* check window settings for sanity -- not perfect yet :) */
	/* these are needed so the mouse doesn't get confused     */

	if (files->height == 0) files->height = LINES - files->y;
	if (files->width < 4) files->width = COLS - files->x;
	if (play->height == 0) play->height = LINES - play->y;
	if (play->width < 4) play->width = COLS - play->x;
//	if (info->height < 6) info->height = 6;
	if (info->width < 4) info->width = COLS - info->x;
	if (menubar->height == 0) menubar->height = 1;
	if (menubar->width == 0) menubar->width = COLS - menubar->x;
//	if (playback->height < 3) playback->height = 3;
	if (playback->width == 0) playback->width = COLS - playback->x;

	active->win = files->win = newwin(files->height, files->width, files->y, files->x);
	info->win = newwin(info->height, info->width, info->y, info->x);
	play->win = newwin(play->height, play->width, play->y, play->x);
	menubar->win = newwin(menubar->height, menubar->width, menubar->y, menubar->x);
	playback->win = newwin(playback->height, playback->width, playback->y, playback->x);
	menubar->input = read_key;

	if (!files->win || !info->win || !play->win || !menubar->win)
		bailout(0);

	/* create the panels in this order to create the initial stack */
	menubar->panel = new_panel(menubar->win);
	info->panel = new_panel(info->win);
	play->panel = new_panel(play->win);
	files->panel = new_panel(files->win);
	playback->panel = new_panel(playback->win);

	keypad(files->win, TRUE);
	keypad(info->win, TRUE);
	keypad(play->win, TRUE);
	keypad(menubar->win, TRUE);

	wbkgd(files->win, colors[FILE_BACK]);
	wbkgd(info->win, colors[INFO_BACK]);
	wbkgd(play->win, colors[PLAY_BACK]);
	wbkgd(menubar->win, colors[MENU_BACK]);
	wbkgd(playback->win, colors[INFO_BACK]);
	menubar->activate(menubar);
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

	doupdate();
	start_mpg_child();
	send_cmd(LOAD, "/usr/local/share/intro.mp3");
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		add_player_descriptor(&fds);
		if (select(FD_SETSIZE, &fds, NULL, NULL, &wait1000) > 0) {
			if (FD_ISSET(0, &fds)){
				active->input(active);
			} else 
				check_player_output(&fds);
		} 
		if (wait1000.tv_usec == 0 ) {
			wait1000.tv_usec = 250000;		
			if (typed_letters_timeout >= 0) {
				if (typed_letters_timeout == 0)
					typed_letters[0] = '\0';
				typed_letters_timeout--;
			}
		}
		
		
	}
	bailout(-1);
}

void
bailout(int sig)
{
	/* Lets get the hell out of here! */
	wclear(stdscr);
	refresh();
	endwin();
	
	switch (sig) {
		case 0: break;
		case 1: fprintf(stderr, "\n\nmjs:error: in and/or outpipe not available OR cannot start mpg123 \n\n\n");
			break;
		case 2: fprintf(stderr, "\n\nmjs:error: starting mpg123 failed !\n\n\n");
			break;
		case 3: fprintf(stderr, "\n\nmjs:error: Forking of mpg123 child proces failed !n\n\n");
			break;
		case 5: fprintf(stderr, "\n\nmjs:warning: There are no command line switches !\n\n");
			fprintf(stderr, " See the file ~/.mjsrc for configuration details.\n");
			break;
		default: fprintf(stderr, "\n\nmjs:error: unknown\n\n\n");
			break;
		}	
	fprintf(stdout, "\n\n MP3 Jukebox System (mjs) v%s\n",VERSION);
	fprintf(stdout, " By Marijn van Galen. (M.P.vanGalen@ITS.TUDelft.nl)\n\n");
	fprintf(stdout, " Based on mms written by Wesley Morgan. (morganw@engr.sc.edu)\n\n\n");
	fprintf(stdout, " Copyright (C) 2002 by Marijn van Galen\n");
	fprintf(stdout, " This program is free software; you can redistribute it and/or modify it\n");
	fprintf(stdout, " under the terms of the GNU General Public License as published by the Free\n");
	fprintf(stdout, " Software Foundation; version 2 of the License.\n\n");
	fprintf(stdout, " This program is distributed in the hope that it will be useful, but WITHOUT\n");
	fprintf(stdout, " ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n");
	fprintf(stdout, " FITNESS FOR A PARTICULAR PURPOSE. \n\n");
	fprintf(stdout, " See the GNU GPL (see LICENSE file) for more details.\n");
	fprintf(stdout, " \n");
	if (pid > 0) {
		pid_t pgrp = getpgid(pid);
		fprintf(stdout, "Cleaning up ...\n\n\n");
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
	wlist *mp3list = files->contents.list;

	c = wgetch(window->win);
	if (c == 27) {
		alt = 1;
		c = wgetch(window->win);
	}

	if (inputline)
		return inputline->parse(inputline, c, alt);
	
	switch (c) {

// Switch between files and playlist window
		case '\t':
			if (active->next)
				change_active(active->next);
			break;
		case KEY_BTAB:
			if (active->prev)
				change_active(active->prev);
			break;

// Move selected forward in playlist			
		case '-':
			if ((active == play) && (!(play->contents.list->selected==play->contents.list->head))) {
				move_backward(play->contents.list);
				play->update(play);
			}
			break;

// Move selected backwards in playlist			
		case '+':
		case '=':
			if ((active == play) && !(play->contents.list->selected == play->contents.list->tail) && (p_status == PLAYING)) {
				move_forward(play->contents.list);
				play->update(play);
			}
			break;

			
		case KEY_DOWN:
		case KEY_UP:
		case KEY_HOME:
		case KEY_END:
		case KEY_PPAGE:
		case KEY_NPAGE:
			if ((window->flags & W_LIST) && info->update(move_selector(window, c))) {
				window->update(window);
			}
			break;


// File selection / directory navigation		
		case KEY_ENTER:
		case '\n':
		case '\r':
			if (active == files) 
				process_return(window->contents.list, c, alt);
			else 
				move_selector(window, c);
			break;

		case KEY_IC:
			if (strcmp(previous_selected, mp3list->selected->fullpath)){ /* we dont want to add the last file multiple times */
				if (previous_selected)
					free (previous_selected);
				previous_selected = strdup(mp3list->selected->fullpath);
		
				if (play->contents.list->playing)
					add_to_playlist(play->contents.list, play->contents.list->playing, mp3list->selected);
				else 
					add_to_playlist(play->contents.list, play->contents.list->selected, mp3list->selected);
				if (conf->c_flags & C_FADVANCE)
					if (info->update(move_selector(files, KEY_DOWN)))
						files->update(files);
			}
			break;

		case KEY_LEFT: 
			if (active == files) {
				char *prevpwd = NULL;
				if (!(mp3list->flags & F_VIRTUAL)) {
					prevpwd = getcwd(NULL, 0);
					if (!strncmp (prevpwd, conf->mp3path, strlen(prevpwd))) {
						free(prevpwd);
						break;
					}
					chdir("../");
				}
				free_list(mp3list->head);
				memset(mp3list, 0, sizeof(wlist));
				mp3list->head = read_mp3_list(mp3list);
				if (mp3list->head)
					sort_songs(mp3list);
				if (prevpwd) {
					while ( strcmp( mp3list->selected->fullpath, prevpwd ) ) 
						move_selector( files, KEY_DOWN );
					free(prevpwd);
				}
				files->update(files);
				play->update(play);
			}
			break;

		case KEY_RIGHT:
			if (active == files) {
				if (!(mp3list->selected->flags & F_DIR )) 
					break;
				chdir(mp3list->selected->fullpath);
				free_list(mp3list->head);
				memset(mp3list, 0, sizeof(wlist));
				mp3list->head = read_mp3_list(mp3list);
				if (mp3list->head) {
					sort_songs(mp3list);
					move_selector( files, KEY_DOWN );
				}
				files->update(files);
				play->update(play);
			}
			break;

			
// remove selected from playlist		
		case KEY_DC:
			if (active == play){
				wlist *playlist = play->contents.list;
				if ((p_status==PLAYING) & (play->contents.list->playing == play->contents.list->selected)){
					jump_forward(play->contents.list);
					play->update(play);
				}
				playlist->selected = delete_file(playlist->selected);
				info->update(play);
				play->update(play);
			}
			break;

// refresh screen			
		case KEY_REFRESH:
			wrefresh(curscr);
			break;

// Exit mjs			
		case KEY_F(1):
			menubar->deactivate(menubar);
			printf_menubar(menubar, EXITPROGRAM);
			update_panels();
			doupdate();
			c = wgetch(window->win);
			if (c == 27) 
				c = wgetch(window->win);
			if ((c == 'y')|(c == 'Y')) 
				bailout(0);
			menubar->activate(menubar);
			update_panels();
			break;

// Clear playlist
		case KEY_F(2):
			menubar->deactivate(menubar);
			printf_menubar(menubar, CLEARPLAYLIST);
			update_panels();
			doupdate();
			c = wgetch(window->win);
			if (c == 27) 
				c = wgetch(window->win);
			if ((c == 'y')|(c == 'Y')) {
				if (p_status) {
					stop_player(play->contents.list);
					clear_play_info();
					}
				free_playlist(play->contents.list);
				play->contents.list = (wlist *)calloc(1, sizeof(wlist));
				play->update(play);
				clear_info();
				info->update(info);
				}
			menubar->activate(menubar);
			update_panels();
			break;
		
// Search in mp3-database
		case KEY_F(3):
			old_active = active;
			active = menubar;
			menubar->inputline = inputline = (Input *)calloc(1, sizeof(Input));
			inputline->win = menubar->win;
			inputline->panel = menubar->panel;
			inputline->x = inputline->y = 0;
			strncpy(inputline->prompt, "Search for:", 39);
			inputline->plen = strlen(inputline->prompt);
			inputline->flen = 70;
			inputline->anchor = inputline->buf;
			inputline->parse = do_inputline;
			inputline->update = update_menu;
			inputline->finish = do_search;
			inputline->complete = filename_complete;
			inputline->pos = 1; 
			inputline->fpos = 1;
			update_menu(inputline);
			break;

// Show last search results
		case KEY_F(4):
			menubar->deactivate(menubar);
			printf_menubar(menubar, SEARCHING);
			update_panels();
			doupdate();
			free_list(mp3list->head);
			free(mp3list);
			mp3list = read_mp3_list_file(NULL,conf->resultsfile);
			if (mp3list->head)
				sort_search(mp3list);
			menubar->activate(menubar);
			info->update(files);
			files->update(files);
			break;	

// Randomize the playlist				
		case KEY_F(5): 
			menubar->deactivate(menubar);
			printf_menubar(menubar, SHUFFLE);
			c = wgetch(window->win);
			if (c == 27) 
				c = wgetch(window->win);
			if ((c == 'y')|(c == 'Y')) {
				randomize_list(play->contents.list);
				active->deactivate(active);
				active->update(active);
				play->activate((active = play));
				play->update(play);
				info->update(play);
				}
			menubar->activate(menubar);
			update_panels();
			break;
				
// Save Playlist
		case KEY_F(6):
			if ((!(conf->c_flags & C_ALLOW_P_SAVE)) | (!(play->contents.list->head)))
				break;
			old_active = active;
			active = menubar;
			clear_menubar(menubar);
			menubar->inputline = inputline = (Input *)calloc(1, sizeof(Input));
			inputline->win = menubar->win;
			inputline->panel = menubar->panel;
			inputline->x = inputline->y = 0;
			strncpy(inputline->prompt, "Save as:", 39);
			inputline->plen = strlen(inputline->prompt);
			inputline->flen = 70;
			inputline->anchor = inputline->buf;
			inputline->parse = do_inputline;
			inputline->update = update_menu;
			inputline->finish = do_save;
			inputline->complete = filename_complete;
			inputline->pos = 1; 
			inputline->fpos = 1;
			update_menu(inputline);
			break;

// Stop the player		
		case KEY_F(7):
			stop_player(play->contents.list);
			p_status=STOPPED;
			break;

// Play / Pause key
		case KEY_F(8):
			switch (p_status) { 
				case STOPPED:
/* fix me */		
					if (!play->contents.list->selected)
						play->contents.list->selected=next_valid(play->contents.list->top,KEY_DOWN);
					jump_to_song(play->contents.list->selected); /* Play */
					break;
				case PLAYING:
					pause_player(play->contents.list); // Pause / Verdergaan 
					break;
				case PAUSED:
					resume_player(play->contents.list); // Pause / Verdergaan 
					break;
				}
			break;

// Skip to previous mp3	in playlist	
		case KEY_F(9):
			if (p_status==PLAYING) {
				jump_backward(play->contents.list);
			}
			break;

// Skip JUMP frames backward
		case KEY_F(10):
			if (p_status)
				send_cmd(JUMP, -conf->jump);
			break;

// Skip JUMP frames forward
		case KEY_F(11):
			if (p_status)
				send_cmd(JUMP, conf->jump);
			break;

// Skip to next mp3 in playlist			
		case KEY_F(12):
			if (p_status==PLAYING) {
				jump_forward(play->contents.list);
			}
			break;

// Jump to directory with matching first letters
		case 'a'...'z':
		case 'A'...'Z':
		case '0'...'9':
		case '.':
		case ' ': {
			flist *prevselected = mp3list->selected;
			if (strlen(typed_letters) < 10) {
				strcat(typed_letters, (char *)&c);
				typed_letters_timeout = 3;
			}
			
			move_selector(files, KEY_HOME);
			
			do {	
				move_selector(files, KEY_DOWN);
				if (mp3list->selected == mp3list->tail) {
					while (mp3list->selected != prevselected)
						move_selector(files, KEY_UP);
					files->update(files);
					doupdate();
					typed_letters_timeout = 0;
					break;
					}
			} while (strncasecmp(mp3list->selected->filename, (char *)&typed_letters, strlen(typed_letters)));
			
			files->update(files);
			break;
		}
		default:
			break;
	}
	doupdate();
	return c;
}

void
process_return(wlist *mp3list, int c, int alt)
{
	if (!mp3list)
		return;

	
	if ((mp3list->selected->flags & F_DIR)) {
		if (!alt) {
			// change to another directory
			char *prevpwd = NULL;
			
			if ((!(mp3list->flags & F_VIRTUAL)) & (!strcmp("../", mp3list->selected->fullpath)))
				prevpwd = getcwd(NULL, 0);
	
			chdir(mp3list->selected->fullpath);
			free_list(mp3list->head);
			memset(mp3list, 0, sizeof(wlist));
			mp3list->head = read_mp3_list(mp3list);
			if (mp3list->head)
				sort_songs(mp3list);
			if (prevpwd) {
				while ( strcmp( mp3list->selected->fullpath, prevpwd ) ) 
					move_selector( files, KEY_DOWN );
				free(prevpwd);
				}
			files->update(files);
		} else {
			// add songs from directory
			if (previous_selected)
				free (previous_selected);
			previous_selected = strdup(mp3list->selected->fullpath);
			add_to_playlist_recursive(play->contents.list, play->contents.list->tail, mp3list->selected);
		}

		
	} else if (mp3list->selected->flags & F_PLAYLIST){
		char *filename = strdup( mp3list->selected->fullpath );
		if  (!alt) // load playlist directly with alt-enter
		{
			free_list(mp3list->head);
			free(mp3list);
			mp3list = read_mp3_list_file(NULL, filename);
			mp3list->selected->flags |= F_SELECTED;
			files->update(files);
		} else {
			play->contents.list = read_mp3_list_file(play->contents.list, filename);
			play->contents.list->selected->flags |= F_SELECTED;
			play->update(play);
		}
		free(filename);	
		menubar->activate(menubar);
		update_panels();

		
	} else // normal mp3
	if (strcmp(previous_selected, mp3list->selected->fullpath)){ /* we dont want to add the last file multiple times */
		if (previous_selected)
			free (previous_selected);
		previous_selected = strdup(mp3list->selected->fullpath);
		
		if (!alt)
			add_to_playlist(play->contents.list, play->contents.list->tail, mp3list->selected);
		else
			add_to_playlist(play->contents.list, play->contents.list->selected, mp3list->selected);

		if (conf->c_flags & C_FADVANCE)
			if (info->update(move_selector(files, KEY_DOWN)))
				files->update(files);
	} 
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
	clear_menubar(menubar);
	update_anchor(inputline);
	my_wnprintw(inputline->win, colors[MENU_TEXT] | A_BLINK, inputline->flen + inputline->plen, "%s", inputline->prompt);
	my_wnprintw(inputline->win, colors[MENU_TEXT], inputline->flen + inputline->plen, " %s", inputline->anchor);
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
	
	if (p_status == STOPPED) {
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
	my_mvwnprintw2(playback->win, 1, 1, colors[TIME], 21,
			" Time: %02d:%02.0f / %02d:%02.0f", minused, secused, minleft, secleft);
			
	if (active->inputline) {
		Input *inputline = active->inputline;
		wmove(inputline->win, inputline->y, inputline->fpos+inputline->plen);
	}
	update_panels();
	doupdate();
}

__inline__ void
clear_play_info(void)
{
	my_mvwnclear(playback->win, 1, 1, playback->width-2);
	update_panels();
	if (conf->c_flags & C_FIX_BORDERS)
		redrawwin(playback->win);
}

int
dummy_update(Window *window)
{
	return 1;
}

static void init_info(void)
{
	WINDOW *win = info->win;

	my_mvwaddstr(win, 1, 2, colors[INFO], "Title :");
	my_mvwaddstr(win, 2, 2, colors[INFO], "Artist:");
	my_mvwaddstr(win, 3, 2, colors[INFO], "Album :");
	update_panels();
	doupdate();
}

