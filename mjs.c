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
flist *prevsel = NULL;

/* some internal functions */
static int	read_key(Window *);
static void	init_info(void);

int
do_save(Input* input)
{
	char *s = malloc(strlen(input->buf)+26);
	active=old_active;
        sprintf(s,"%s/%s.mms", conf->playlistpath, input->buf);
	write_mp3_list_file(play->contents.list,s);
	free(input);
	menubar->activate(menubar);
	menubar->inputline = NULL;
	curs_set(0);
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
		memset(mp3list, 0, sizeof(wlist));		
		mp3list = read_mp3_list_file(mp3list, conf->resultsfile);
		if (mp3list->head)
			sort_search(mp3list);
		files->update(files);
		}
	free(input);
	menubar->activate(menubar);
	menubar->inputline = NULL;
	curs_set(0);
	update_panels();
	doupdate();
	return 1;
}


int
main(int argc, char *argv[])
{
	wlist *mp3list = NULL;
	fd_set fds;
	
	srand(time(NULL));
	
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
	if (argc>1)
		bailout(0);
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

	menubar->activate = std_bottom_line;
	menubar->deactivate = clear_bottom_line;

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
//
//exit(0);
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
//

	doupdate();
	start_mpg_child();
	send_cmd(LOAD, "/usr/local/share/intro.mp3");
	for (;;) {
		    struct timeval wait1700 = {0, 1700000};
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		add_player_descriptor(&fds);
		if (select(FD_SETSIZE, &fds, NULL, NULL, &wait1700) > 0) {
			if (FD_ISSET(0, &fds)){
				active->input(active);
				}

			else 
				check_player_output(&fds);
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
		case KEY_SLEFT:
		case '-':
			if ((active == play) && (!(play->contents.list->selected==play->contents.list->head))) {
				move_backward(play->contents.list);
//				if (p_status)
//					calculate_duration(play->contents.list->playing);
				play->update(play);
				doupdate();
			}
			break;

// Move selected backwards in playlist			
		case '+':
		case '=':
			if ((active == play) && !(play->contents.list->selected==play->contents.list->tail)) {
				move_forward(play->contents.list);
//				if (p_status)
//					calculate_duration(play->contents.list->playing);
				play->update(play);
				doupdate();
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
				doupdate();
			}
			break;


// File selection / directory navigation		
		case KEY_ENTER:
		case KEY_RIGHT:
		case KEY_LEFT:
		case '\n':
		case '\r':
			process_return(window->contents.list, c, alt);
			break;

// remove selected from playlist		
		case KEY_DC:
			if (active == play){
				wlist *playlist = play->contents.list;				
				playlist->selected = delete_file(play, playlist->selected);
//				if (p_status)
//					calculate_duration(play->contents.list->playing);
				info->update(play);
				play->update(play);
				doupdate();
			}
			break;

// refresh screen			
		case KEY_REFRESH:
			wrefresh(curscr);
			break;

// Exit mms			
		case KEY_F(1):
			menubar->deactivate(menubar);
			my_mvwaddstr(menubar->win, 0, 10, colors[MENU_TEXT], "Are you sure you want to reset this program ? (y/n)");
			update_panels();
			doupdate();
			c = wgetch(window->win);
			if (c == 27) 
				c = wgetch(window->win);
			if ((c == 'y')|(c == 'Y')) 
				bailout(0);
			menubar->activate(menubar);
			curs_set(0);
			update_panels();
			doupdate();
			break;

// Clear playlist
		case KEY_F(2):
			menubar->deactivate(menubar);
			my_mvwaddstr(menubar->win, 0, 10, colors[MENU_TEXT], "Are you sure you want to clear the playlist ? (y/n)");
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
				}
			menubar->activate(menubar);
			curs_set(0);
			info->update(play);
			update_panels();
			doupdate();	
			break;
		
// Search in mp3-database
		case KEY_F(3):
			old_active = active;
			active = menubar;
			wbkgd(menubar->win, 0);
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
			curs_set(1);
			update_menu(inputline);
			doupdate();
			break;

// Show last search results
		case KEY_F(4):
			free_list(mp3list->head);
			memset(mp3list, 0, sizeof(wlist));
			mp3list = read_mp3_list_file(mp3list,conf->resultsfile);
			if (mp3list->head)
				sort_search(mp3list);
			menubar->activate(menubar);
			info->update(files);
			files->update(files);
			doupdate();
			break;	

// Randomize the playlist				
		case KEY_F(5): 
			menubar->deactivate(menubar);
			my_mvwaddstr(menubar->win, 0, 10, colors[MENU_TEXT], "Shuffle Playlist ? (y/n)");
			update_panels();
			doupdate();
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
			curs_set(0);
			update_panels();
			doupdate();	
			break;
				
// Save Playlist
		case KEY_F(6):
			if ((!(conf->c_flags & C_ALLOW_P_SAVE)) | (!(play->contents.list->head)))
				break;
			old_active = active;
			active = menubar;
			wbkgd(menubar->win, 0);
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
			curs_set(1);
			update_menu(inputline);
			doupdate();
			break;

// Stop the player		
		case KEY_F(7):
			if (p_status) {
				stop_player(play->contents.list);
				clear_play_info();
				doupdate();
				}
			p_status=STOPPED;
//			calculate_duration(play->contents.list->head);
			break;

// Play / Pause key
		case KEY_F(8):
			switch (p_status) { 
				case STOPPED:
/* fix me */		
					if (!play->contents.list->selected)
						play->contents.list->selected=next_valid(play,play->contents.list->top,KEY_DOWN);
//					calculate_duration(play->contents.list->selected);
					jump_to_song(play->contents.list->selected); /* Play */
					doupdate(); 
					break;
				case PLAYING:
					pause_player(play->contents.list); // Pause / Verdergaan 
					play->update(play);
					doupdate();
					break;
				case PAUSED:
					resume_player(play->contents.list); // Pause / Verdergaan 
					play->update(play);
					doupdate();
					break;
				}
			break;

// Skip to previous mp3	in playlist	
		case KEY_F(9):
			if (p_status==PLAYING) {
				jump_backward(play->contents.list);
				files->update(files);
				doupdate();
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
				play->update(play);
				doupdate();
			}
			break;

// Jump to directory with matching first letter
		case 'a'...'z':
		case 'A'...'Z': {
			if (!strncasecmp(mp3list->selected->filename+1, (char *)&c,1)) {	// At least one dirname starting with S
				do {	
					move_selector(files, KEY_DOWN);
					if (mp3list->selected == mp3list->tail) 
						move_selector(files, KEY_HOME);
					}
				while (strncasecmp(mp3list->selected->filename+1, (char *)&c,1));
				}
			else {
				move_selector(files, KEY_HOME);
				do {	
					move_selector(files, KEY_DOWN);
					if (mp3list->selected == mp3list->tail) {
						move_selector(files, KEY_HOME);
						files->update(files);
						doupdate();
						break;
						}
					}
				while (strncasecmp(mp3list->selected->filename+1, (char *)&c,1));
				}

			files->update(files);
			doupdate();
			}
			break;
		default:
			break;
	}
	return c;
}

void
process_return(wlist *mp3list, int c, int alt)

{
	if (!mp3list)
		return;
	if (active == files) {
		if ((mp3list->selected->flags & F_DIR) | (c == KEY_LEFT)) {
			char *prevpwd = NULL;
			if (c == KEY_LEFT) {
				move_selector(files, KEY_HOME);
				}
			if (!(mp3list->selected->flags & F_SEARCHDIR)) 
				if (!strcmp("../", mp3list->selected->fullpath))
                			prevpwd = getcwd(NULL, 0);
			chdir(mp3list->selected->fullpath);
			free_list(mp3list->head);
			memset(mp3list, 0, sizeof(wlist));
			mp3list->head = read_mp3_list(mp3list);
			if (mp3list->head)
				sort_songs(mp3list);
			if (prevpwd) {
				while (strcmp(mp3list->selected->fullpath, prevpwd)) {
					move_selector(files, KEY_DOWN);
				}
				free(prevpwd);
			}
			files->update(files);
		} else 	
			if (mp3list->selected->flags & F_PLAYLIST){
				char *filename=strdup(mp3list->selected->fullpath);
// add mp3's in file to playlist
				if (conf->c_flags & C_P_TO_F) {
					free_list(mp3list->head);
					memset(mp3list, 0, sizeof(wlist));
					mp3list = read_mp3_list_file(mp3list,filename);
					mp3list->selected->flags |= F_SELECTED;
				} else {
					play->contents.list = read_mp3_list_file(play->contents.list,filename);
					play->contents.list->selected->flags |= F_SELECTED;
				}
				menubar->activate(menubar);
				info->update(files);
				files->update(files);		
				}
			else	if ((prevsel != mp3list->selected) & (c != KEY_RIGHT)){ /* we dont want to add the last file multiple times */
					prevsel = mp3list->selected;
					if (alt)
						add_to_playlist(play->contents.list, play->contents.list->selected, mp3list->selected);
					else
						add_to_playlist(play->contents.list, play->contents.list->tail, mp3list->selected);
					if (conf->c_flags & C_FADVANCE)
						if (info->update(move_selector(files, KEY_DOWN)))
							files->update(files);
				} 
		play->update(play);
		}
	doupdate();
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
	long minplaylist;
	double secleft, secused,secplaylist;
	
	minleft = (int)message->remaining / 60;
	secleft = message->remaining - minleft*60;
	minused = (int)message->elapsed / 60;
	secused = message->elapsed - minused*60;
	if (conf->c_flags & C_SHOW_P_LENGTH) {
		minplaylist = (play->contents.list->duration+(int)message->remaining) / 60;
		secplaylist = (double)play->contents.list->duration + message->remaining- minplaylist*60;
		my_mvwnprintw2(playback->win, 1, 2, colors[UNSELECTED], playback->width-3,
			"Time: %02d:%02.0f / %02d:%02.0f  List: %02d:%02.0f",
			minused, secused, minleft, secleft, minplaylist, secplaylist);
		}
	else {
		my_mvwnprintw2(playback->win, 1, 2, colors[UNSELECTED], playback->width-3,
			"Time: %02d:%02.0f / %02d:%02.0f",
			minused, secused, minleft, secleft);
		}
			
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
	my_mvwnclear(playback->win, 1, 2, playback->width-3);
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

	my_mvwaddstr(win, 1, 2, colors[UNSELECTED], "Title :");
	my_mvwaddstr(win, 2, 2, colors[UNSELECTED], "Artist:");
	my_mvwaddstr(win, 3, 2, colors[UNSELECTED], "Album :");
	update_panels();
}

