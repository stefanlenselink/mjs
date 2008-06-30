#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "mjs.h"
#include "playlist.h"
#include "gui/window.h"
#include "misc.h"
#include "files.h"
#include "mpgcontrol.h"
#include "config.h"
#include "inputline.h"
#include "extern.h"
#include "list.h"
#include "lastfm.h"

/*
 * intialize the external variables 
 */
Window *files, *info, *play, *active, *menubar, *old_active;
Window *playback, *question;
Config *conf;
pid_t pid;
static struct sigaction handler;
char *previous_selected;	// previous selected number
int p_status = STOPPED;
char typed_letters[10] = "\0";	// letters previously typed when jumping
int typed_letters_timeout = 0;	// timeout for previously typed letters

/*TODO LastFM LastFMHandshake * lastFMHandshake;*/

/*
 * some internal functions 
 */
static int read_key (Window *);
static void init_info (Window *);
static void timer_handler(int signum);

static void timer_handler(int signum)
{
//	active->update(active);
//	menubar->update(menubar);
//	wrefresh (curscr);
//	refresh();
  fprintf(stderr, "UpDate!\n");
  clear_menubar(menubar);
	std_menubar(menubar);
    doupdate ();
    //refresh();
	//wrefresh (curscr);
}


int
do_save (Input * input)
{
	char *s = malloc (strlen (input->buf) + 26);
	active = old_active;
	sprintf (s, "%s/%s.mjs", conf->playlistpath, input->buf);
	write_mp3_list_file (play->contents.list, s);
	free (s);
	free (input);
	menubar->activate (menubar);
	menubar->inputline = NULL;
	doupdate ();
	return 1;
}

int
do_search (Input * input)
{
	pid_t childpid;
	wlist *mp3list = files->contents.list;
	active = old_active;
	if (!((*input->buf == ' ') || (*input->buf == '\0'))) {
		handler.sa_handler = SIG_DFL;
		handler.sa_flags = SA_ONESHOT;
		sigaction (SIGCHLD, &handler, NULL);
		errno = 0;
		if (!(childpid = fork ())) {
			errno = 0;
			execlp ("findmp3", "findmp3", input->buf, conf->resultsfile, (char *) NULL);
			exit (3);
		}
		if (errno)
			exit (3);

		waitpid (childpid, NULL, 0);

		handler.sa_handler = (SIGHANDLER) unsuspend;
		handler.sa_flags = SA_RESTART;
		sigaction (SIGCONT, &handler, NULL);
		handler.sa_handler = (SIGHANDLER) restart_mpg_child;
		handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
		sigaction (SIGCHLD, &handler, NULL);
		if (!(mp3list->flags & F_VIRTUAL))
			dirstack_push(mp3list->from, mp3list->selected->filename);
		read_mp3_list (mp3list, conf->resultsfile, L_SEARCH);
		files->update (files);
		info->update(info);
	} else
		menubar->activate (menubar);
	
	free (input);
	menubar->inputline = NULL;
	files->update (files);
	doupdate ();
	return 1;
}

int
main (int argc, char *argv[])
{
	wlist *mp3list = NULL;
	struct timeval wait1000 = { 0, 1000000 };
	int timeout = 0;
	fd_set fds;

	previous_selected = strdup ("\0");
	srand (time (NULL));
	
	memset(&handler, 0, sizeof(struct sigaction));

    /* Ignore the SIGQUIT signals a.k.a pressing Prt Scr */
    handler.sa_handler = SIG_IGN;
	handler.sa_flags = 0;
	sigaction (SIGQUIT, &handler, NULL);
    
	handler.sa_handler = (SIGHANDLER) bailout;
	sigaction (SIGINT, &handler, NULL);
	handler.sa_handler = (SIGHANDLER) unsuspend;
	handler.sa_flags = SA_RESTART;
	sigaction (SIGCONT, &handler, NULL);
	handler.sa_handler = (SIGHANDLER) restart_mpg_child;
	handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction (SIGCHLD, &handler, NULL);
	
    
    
	/*Uitgezet door Bug tijdens de Owee*/
	struct itimerval rttimer;
	struct itimerval old_rttimer;
	
	signal(SIGALRM,timer_handler);
	rttimer.it_value.tv_sec     = 3; /* A signal will be sent 250 Milisecond*/
	rttimer.it_value.tv_usec    = 0; /*  from when this is called */
	rttimer.it_interval.tv_sec  = 3; /* If the timer is not reset, the */
	rttimer.it_interval.tv_usec = 0; /*  signal will be sent every 250 Milisecond */
	
	setitimer (ITIMER_REAL, &rttimer, &old_rttimer);

	if (!initscr ())
		exit (1);
	curs_set (0);
	leaveok (stdscr, TRUE);
	cbreak ();
	noecho ();
	use_default_colors ();
	start_color ();
	nonl ();
	init_ansi_pair ();

//	if (argc > 1)
//		bailout (5);

	/*
	 * malloc() for the windows and set up our initial callbacks ... 
	 */

	info = calloc (1, sizeof (Window));
	play = calloc (1, sizeof (Window));
	playback = calloc (1, sizeof (Window));
	menubar = calloc (1, sizeof (Window));
	active = files = calloc (1, sizeof (Window));
	
	/*
	 * reading the config must take place AFTER initscr() and friends 
	 */

	info->update = update_info;
	info->activate = active_win;
	info->deactivate = inactive_win;
	info->prev = files;
	info->next = play;
	info->input = read_key;
	info->flags |= W_RDONLY;

	/*
	 * in theory, this window should NEVER be active, but just in case ... 
	 */
	playback->update = update_info;	// generic dummy update 
	playback->activate = active_win;
	playback->deactivate = inactive_win;
	playback->prev = NULL;	// can't tab out of this! 
	playback->next = NULL;
	playback->input = read_key;
	playback->flags |= W_RDONLY;
	playback->yoffset = 1;

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
	menubar->update = std_menubar;
	menubar->deactivate = clear_menubar;

	/*
	 * now we have initialized most of the window stuff, read our config 
	 */

	conf = calloc (1, sizeof (Config));
	strncpy (conf->mpgpath, MPGPATH, 255);
	read_config (conf);

	/*
	 * check window settings for sanity -- not perfect yet :) 
	 */

	if (files->height == 0)
		files->height = LINES - files->y;
	if (files->width < 4)
		files->width = COLS - files->x;
	if (play->height == 0)
		play->height = LINES - play->y;
	if (play->width < 4)
		play->width = COLS - play->x;
	if (info->width < 4)
		info->width = COLS - info->x;
	if (menubar->height == 0)
		menubar->height = 1;
	if (menubar->width == 0)
		menubar->width = COLS - menubar->x;
	if (playback->height < 6)
		playback->height = 2;
	if (playback->width == 0)
		playback->width = COLS - playback->x;

	active->win = files->win = newwin (files->height, files->width, files->y, files->x);
	info->win = newwin (info->height, info->width, info->y, info->x);
	play->win = newwin (play->height, play->width, play->y, play->x);
	menubar->win = newwin (menubar->height, menubar->width, menubar->y, menubar->x);
	playback->win = newwin (playback->height, playback->width, playback->y, playback->x);
	menubar->input = read_key;

	if (!files->win || !info->win || !play->win || !menubar->win)
		bailout (0);

	/*
	 * create the panels in this order to create the initial stack 
	 */

	menubar->panel = new_panel (menubar->win);
	info->panel = new_panel (info->win);
	play->panel = new_panel (play->win);
	files->panel = new_panel (files->win);
	playback->panel = new_panel (playback->win);

	keypad (files->win, TRUE);
	keypad (info->win, TRUE);
	keypad (play->win, TRUE);
	keypad (menubar->win, TRUE);

	wbkgd (stdscr, colors[FILE_WINDOW]);
	wbkgd (files->win, colors[FILE_WINDOW]);
	wbkgd (info->win, colors[INFO_WINDOW]);
	wbkgd (play->win, colors[PLAY_WINDOW]);
	wbkgd (menubar->win, colors[MENU_WINDOW]);
	wbkgd (playback->win, colors[PLAYBACK_WINDOW]);

	files->contents.list = mp3list = (wlist *) calloc (1, sizeof (wlist));
    read_mp3_list (mp3list, conf->mp3path, L_NEW);
	info->contents.show = &mp3list->selected;
	play->contents.list = (wlist *) calloc (1, sizeof (wlist));
	wlist_clear(play->contents.list);

	if (argc > 1)
		read_mp3_list_array(play->contents.list, argc, argv);
	else if (conf->c_flags & C_P_SAVE_EXIT)
		read_mp3_list_file (play->contents.list, "/home/mvgalen/.previous_playlist.mjs", 1);

	menubar->activate (menubar);
	init_info (info);
	init_info (playback);
	play->deactivate (play);

	info->deactivate (info);
	active->activate (active);
	playback->deactivate (playback);

	play->update(play);
	files->update (files);
	info->update(info);
	mpgreturn ret;
	ret.elapsed = 0.0;
	ret.remaining = 0.0;
	show_playinfo(&ret);
	doupdate ();
	start_mpg_child ();
	send_cmd (LOAD, "/usr/local/share/intro.mp3");
/* TODO LastFM    lastFMHandshake = malloc(sizeof(LastFMHandshake));*/
/* TODO LastFM   initLastFM(lastFMHandshake, "L-Tech", "xxx");*/
    
    //Main loop
	for (;;) {
		FD_ZERO (&fds);
		FD_SET (0, &fds);
		add_player_descriptor (&fds);
		if (select (FD_SETSIZE, &fds, NULL, NULL, &wait1000) > 0) {
			if (FD_ISSET (0, &fds)) {
				if (active != menubar)
					info->contents.show = &active->contents.list->selected;
				info->update(info);
				active->input (active);
				timeout = 0;
			} else 
				check_player_output (&fds);
		}
		if (wait1000.tv_usec == 0) {
			wait1000.tv_usec = 200000;
			if (typed_letters_timeout >= 0) {
				if (typed_letters_timeout == 0)
					typed_letters[0] = '\0';
				typed_letters_timeout--;
			}
		}
	}
	bailout (-1);
}

void
bailout (int sig)
{
	/*
	 * Lets get the hell out of here! 
	 */
	handler.sa_handler = SIG_IGN;
	handler.sa_flags = 0;
	sigaction (SIGINT, &handler, NULL);

	wclear (stdscr);
	refresh ();
	endwin ();


	switch (sig) {
	case 0:
		if (((conf->c_flags & C_P_SAVE_EXIT) > 0) & (play->contents.list->head != NULL)){
			write_mp3_list_file (play->contents.list, "/home/mvgalen/.previous_playlist.mjs");
		}
		if (play->contents.list) {
			wlist_clear (play->contents.list);
			free (play->contents.list);
		}
		if (files->contents.list){
			wlist_clear (files->contents.list);
			free (files->contents.list);
		}

		free (info);
		free (play);
		free (playback);
		free (menubar);
		free (files);
	
		free (conf);
		break;
	case 1:
		fprintf (stderr, "\n\nmjs:error: in and/or outpipe not available OR cannot start mpg123 \n\n\n");
		break;
	case 2:
		fprintf (stderr, "\n\nmjs:error: starting mpg123 failed !\n\n\n");
		break;
	case 3:
		fprintf (stderr, "\n\nmjs:error: Forking of mpg123 child proces failed !n\n\n");
		break;
	case 5:
		fprintf (stderr, "\n\nmjs:warning: There are no command line switches !\n\n");
		fprintf (stderr, " See the file ~/.mjsrc for configuration details.\n");
		break;
	default:
		fprintf (stderr, "\n\nmjs:error: unknown\n\n\n");
		break;
	}
	fprintf (stdout, "\n\n MP3 Jukebox System (mjs) v%s\n", "4.0"/* TODO VERSION*/);
	fprintf (stdout, " By Marijn van Galen. (M.P.vanGalen@ITS.TUDelft.nl)\n\n");
	fprintf (stdout, " Based on mms written by Wesley Morgan. (morganw@engr.sc.edu)\n\n\n");
	fprintf (stdout, " Copyright (C) 2002 by Marijn van Galen\n");
	fprintf (stdout, " This program is free software; you can redistribute it and/or modify it\n");
	fprintf (stdout, " under the terms of the GNU General Public License as published by the Free\n");
	fprintf (stdout, " Software Foundation; version 2 of the License.\n\n");
	fprintf (stdout, " This program is distributed in the hope that it will be useful, but WITHOUT\n");
	fprintf (stdout, " ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n");
	fprintf (stdout, " FITNESS FOR A PARTICULAR PURPOSE. \n\n");
	fprintf (stdout, " See the GNU GPL (see LICENSE file) for more details.\n");
	fprintf (stdout, " \n");
	if (pid > 0) {
		pid_t pgrp = getpgid (pid);
		fprintf (stdout, "Cleaning up ...\n\n\n");
		fflush (stdout);
		handler.sa_handler = SIG_DFL;
		handler.sa_flags = 0;
		sigaction (SIGCHLD, &handler, NULL);
		send_cmd(QUIT);	

		// kill the entire process group, for the buffering child 
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		killpg (pgrp, SIGTERM);
#else
		kill(-pgrp, SIGTERM);
#endif // *BSD
		waitpid (-pid, NULL, 0);	// be safe and avoid a zombie 
	}
	exit (sig);
}

static int
read_key (Window * window)
{
	int c, alt = 0;
	Input *inputline = window->inputline;
	wlist *mp3list = files->contents.list;

	c = wgetch (window->win);
	if (c == 27) {
		alt = 1;
		c = wgetch (window->win);
	}

	if (inputline)
		return inputline->parse (inputline, c, alt);

	switch (c) {

	case '\t':
		// Switch between files and playlist window
		if (active->next)
			change_active (active->next);
		break;

	case KEY_BTAB:
		// Switch between files and playlist window
		if (active->prev)
			change_active (active->prev);
		break;

	case '-':
		// Move selected forward in playlist                    
		if ((active == play) && !(play->contents.list->selected == play->contents.list->head)) {
			move_backward (play->contents.list);
			play->update (play);
		}
		break;

	case '+':
	case '=':
		// Move selected backwards in playlist                  
		if ((active == play) && !(play->contents.list->selected == play->contents.list->tail)) {
			move_forward (play->contents.list);
			play->update (play);
		}
		break;


	case KEY_DOWN:
	case KEY_UP:
	case KEY_HOME:
	case KEY_END:
	case KEY_PPAGE:
	case KEY_NPAGE:
		if ((window->flags & W_LIST)
		    && (move_selector (window, c))) {
			window->update (window);
			info->update(info);
		}
		break;


	case KEY_ENTER:
	case '\n':
	case '\r':
		// File selection / directory navigation                
		if (active == files)
			process_return (window->contents.list, alt);
		else {
			move_selector (window, c);
			play->update(play);
		}
		break;

	case KEY_IC:
		if (!((mp3list->selected->flags & F_DIR) | (mp3list->selected->flags & F_PLAYLIST)))
			if (strcmp (previous_selected, mp3list->selected->fullpath)) {	// we dont want to add the last file multiple times 
				if (previous_selected)
					free (previous_selected);
				previous_selected = strdup (mp3list->selected->fullpath);

				if (play->contents.list->playing)
					add_to_playlist (play->contents.list, play->contents.list->playing, mp3list->selected);
				else
					add_to_playlist (play->contents.list, play->contents.list->selected, mp3list->selected);
				if (conf->c_flags & C_FADVANCE)
					if (move_selector (files, KEY_DOWN)) {
						files->update (files);
						info->update (info);
					}
				play->update (play);
			}
		break;

	case KEY_LEFT:
		// leave directory
		if ((active == files) && !dirstack_empty()) {
			char * filename = strdup(dirstack_filename());
			char * fullpath = strdup(dirstack_fullpath());
			dirstack_pop();
			read_mp3_list (mp3list, fullpath, L_NEW);
			while (strcmp (mp3list->selected->filename, filename))
				move_selector (files, KEY_DOWN);
			files->update (files);
			info->update (info);
			free(filename);
			free(fullpath);
		}
		break;

	case KEY_RIGHT:
// enter directory
		if (active == files) {
			if ((mp3list->selected->flags & (F_DIR|F_PLAYLIST))
				&& (strncmp(mp3list->selected->filename, "../",3))) {
				if (!(mp3list->flags & F_VIRTUAL)) 
					dirstack_push(mp3list->from, mp3list->selected->filename);
				read_mp3_list (mp3list, mp3list->selected->fullpath, L_NEW);
				if (mp3list->head) 
					move_selector (files, KEY_DOWN);
			}
		
			files->update (files);
			info->update (info);
		}
		break;


	case KEY_DC:
		// remove selected from playlist                
		if ((active == play) && (play->contents.list->selected)) {
			wlist *playlist = play->contents.list;
			if ((p_status == PLAYING) & (play->contents.list->playing == play->contents.list->selected)) {
				play_next_song (play->contents.list);
				play->update (play);
			}
			wlist_del(playlist, playlist->selected);
			info->update (info);
			play->update (play);
		}
		break;

	case KEY_REFRESH:
	case '~':
	case '`':
		// refresh screen                       
		wrefresh (curscr);
		break;

	case KEY_F (1):
		// Exit mjs                     
		menubar->deactivate (menubar);
		printf_menubar (menubar, EXITPROGRAM);
		c = wgetch (window->win);
		if (c == 27)
			c = wgetch (window->win);
		if ((c == 'y') | (c == 'Y'))
			bailout (0);
		menubar->activate (menubar);
		update_panels ();
		break;

	case KEY_F (2):
		// Clear playlist
		menubar->deactivate (menubar);
		printf_menubar (menubar, CLEARPLAYLIST);
		c = wgetch (window->win);
		if (c == 27)
			c = wgetch (window->win);
		if ((c == 'y') | (c == 'Y')) {
			if (p_status) {
				stop_player (play->contents.list);
				clear_play_info ();
			}

			wlist_clear(play->contents.list);

			play->update (play);
			clear_info ();
			info->update (info);
		}
		menubar->activate (menubar);
		update_panels ();
		break;

	case KEY_F (3):
		// Search in mp3-database
		old_active = active;
		active = menubar;
		menubar->inputline = inputline = (Input *) calloc (1, sizeof (Input));
		inputline->win = menubar->win;
		inputline->panel = menubar->panel;
		inputline->x = inputline->y = 0;
		strncpy (inputline->prompt, "Search for:", 39);
		inputline->plen = strlen (inputline->prompt);
		inputline->flen = 70;
		inputline->anchor = inputline->buf;
		inputline->parse = do_inputline;
		inputline->update = update_menu;
		inputline->finish = do_search;
		inputline->complete = filename_complete;
		inputline->pos = 1;
		inputline->fpos = 1;
		update_menu (inputline);
		break;

	case KEY_F (4):
		// Show last search results
		if (!(mp3list->flags & F_VIRTUAL))
			dirstack_push(mp3list->from, mp3list->selected->filename);
		read_mp3_list (mp3list, conf->resultsfile, L_SEARCH);
		info->update (info);
		files->update (files);
		break;

	case KEY_F (5):
		// Randomize the playlist                               
		menubar->deactivate (menubar);
		printf_menubar (menubar, SHUFFLE);
		c = wgetch (window->win);
		if (c == 27)
			c = wgetch (window->win);
		if ((c == 'y') | (c == 'Y')) {
			randomize_list (play->contents.list);
			play->update (play);
			info->update (info);
		}
		menubar->activate (menubar);
		break;

	case KEY_F (6):
		// Save Playlist
		if ((!(conf->c_flags & C_ALLOW_P_SAVE)) | (!(play->contents.list->head)))
			break;
		old_active = active;
		active = menubar;
		clear_menubar (menubar);
		menubar->inputline = inputline = (Input *) calloc (1, sizeof (Input));
		inputline->win = menubar->win;
		inputline->panel = menubar->panel;
		inputline->x = inputline->y = 0;
		strncpy (inputline->prompt, "Save as:", 39);
		inputline->plen = strlen (inputline->prompt);
		inputline->flen = 70;
		inputline->anchor = inputline->buf;
		inputline->parse = do_inputline;
		inputline->update = update_menu;
		inputline->finish = do_save;
		inputline->complete = filename_complete;
		inputline->pos = 1;
		inputline->fpos = 1;
		update_menu (inputline);
		break;

	case KEY_F (7):
		// Stop the player              
		stop_player (play->contents.list);
		p_status = STOPPED;
		break;

	case KEY_F (8):
		// Play / Pause key
		switch (p_status) {
		case STOPPED:
			// fix me 
			if (!play->contents.list->selected)
				play->contents.list->selected = next_valid (play->contents.list, play->contents.list->top, KEY_DOWN);
			jump_to_song (play->contents.list, play->contents.list->selected);	// Play 
			break;
		case PLAYING:
			pause_player (play->contents.list);	// Pause / Verdergaan 
			break;
		case PAUSED:
			resume_player (play->contents.list);	// Pause / Verdergaan 
			break;
		}
		break;

	case KEY_F (9):
		// Skip to previous mp3 in playlist     
		if (p_status == PLAYING) {
			play_prev_song (play->contents.list);
		}
		break;

	case KEY_F (10):
		// Skip JUMP frames backward
		if (p_status)
			send_cmd (JUMP, -conf->jump);
		break;

	case KEY_F (11):
		// Skip JUMP frames forward
		if (p_status)
			send_cmd (JUMP, conf->jump);
		break;

	case KEY_F (12):
		// Skip to next mp3 in playlist                 
		if (p_status == PLAYING) {
			play_next_song (play->contents.list);
		}
		break;

	case 'a'...'z':
	case 'A'...'Z':
	case '0'...'9':
	case '.':
	case ' ':
		// Jump to directory with matching first letters
		if (active == files){
			flist *ftmp = mp3list->head;
			int n = 0;
			if (strlen (typed_letters) < 10) { // add the letter to the string and reset the timeout
				strcat (typed_letters, (char *) &c);
				typed_letters_timeout = 4;
			}
			
			while (strncasecmp (ftmp->filename, (char *) &typed_letters, strlen (typed_letters))) {
				if (ftmp == mp3list->tail) { // end of the list reached without result
					ftmp = NULL;
					break;
				}
				ftmp = ftmp->next;
				n++;
			}

			if (ftmp) { // match found
				mp3list->selected = ftmp;
				mp3list->where = n;
				files->update(files);
			}
		}
		break;
		
	case KEY_BACKSPACE:
		if (active == files) {
			if (strlen(typed_letters) > 1) {
				typed_letters[strlen(typed_letters) - 1] = '\0';
				typed_letters_timeout = 4;
			} else 	if (strlen(typed_letters) == 1) {
				typed_letters[0] = '\0';
				typed_letters_timeout = 0;
			}
		}
		break;
		
	default:
		break;
	}
	doupdate ();
	return c;
}

void
process_return (wlist * fileslist, int alt)
{

	wlist *playlist = play->contents.list;
	if (!fileslist)
		return;



	if ((fileslist->selected->flags & F_DIR)) {
		if (!alt) {
			// change to another directory

			if (!strcmp ("../", fileslist->selected->filename)) {
				char * filename = strdup(dirstack_filename());
				char * fullpath = strdup(dirstack_fullpath());
				dirstack_pop();
				read_mp3_list (fileslist, fullpath, L_NEW);
				while (strcmp (fileslist->selected->filename, filename))
					move_selector (files, KEY_DOWN);
				free(filename);
				free(fullpath);
			} else {
				if (!(fileslist->flags & F_VIRTUAL)) 
					dirstack_push(fileslist->from, fileslist->selected->filename);
				read_mp3_list (fileslist, fileslist->selected->fullpath, L_NEW);
			}

			files->update (files);
		} else {
			// add songs from directory
			if (previous_selected)
				free (previous_selected);
				previous_selected = strdup (fileslist->selected->fullpath);
			if ((!(fileslist->flags & F_VIRTUAL)) & (strcmp ("../", fileslist->selected->fullpath))) {
				add_to_playlist_recursive (playlist, playlist->tail, fileslist->selected);
				play->update(play);
			}
		}


	} else if (fileslist->selected->flags & F_PLAYLIST) {
//		if ((alt > 0) ^ ((conf->c_flags & C_P_TO_F) > 0))	// load playlist directly with alt-enter
		if (!alt)
		{
			dirstack_push(fileslist->from, fileslist->selected->filename);
			read_mp3_list (fileslist, fileslist->selected->fullpath, L_NEW);
			files->update (files);
		} else {
//			read_mp3_list (playlist, fileslist->selected->fullpath, L_APPEND);
			add_to_playlist_recursive (playlist, playlist->tail, fileslist->selected);
			play->update (play);
		}

		update_panels ();


	} else			// normal mp3
	if (strcmp (previous_selected, fileslist->selected->fullpath)) {	// we dont want to add the last file multiple times 
		if (previous_selected)
			free (previous_selected);
		previous_selected = strdup (fileslist->selected->fullpath);

		if (!alt)
			add_to_playlist (playlist, playlist->tail, fileslist->selected);
		else
			add_to_playlist (playlist, playlist->selected, fileslist->selected);
		play->update(play);
		if (conf->c_flags & C_FADVANCE)
			if (move_selector (files, KEY_DOWN)) {
				info->update (info);
				files->update (files);
			}
	}

}

void
unsuspend (int sig)
{
	wrefresh (curscr);
	curs_set (0);
}

int
update_menu (Input * inputline)
{
	clear_menubar (menubar);
	update_anchor (inputline);
	my_wnprintw (inputline->win, colors[MENU_TEXT] | A_BLINK, inputline->flen + inputline->plen, "%s", inputline->prompt);
	my_wnprintw (inputline->win, colors[MENU_TEXT], inputline->flen + inputline->plen, " %s", inputline->anchor);
	update_panels ();
	doupdate ();
	return 1;
}

void
update_status (void)
{
	FILE *logfile;
	time_t timevalue;
	wlist *list = play->contents.list;
	if (!list)
		return;

	logfile = fopen(conf->logfile,"a");

	if (p_status == STOPPED) {
		clear_play_info ();
		if (list->playing!=NULL) {
			if (logfile!=NULL) {
				timevalue = time(NULL);
				fprintf(logfile,"%.24s\t%s\n",ctime(&timevalue), list->playing->fullpath);
				fclose(logfile);
			}
            if (list->playing->flags & F_HTTP)
                jump_to_song(list, list->playing);
            else
                play_next_song (list);
		}
		doupdate ();
	}
}

void
show_playinfo (mpgreturn * message)
{
	playback->contents.show = &play->contents.list->playing;
	playback->update(playback);
	my_mvwnprintw2 (playback->win, 1, 1, colors[PLAYBACK_TEXT], 35, " Time  : %02d:%02d / %02d:%02d (%02d:%02d)", 
		(int)message->elapsed / 60, ((int)message->elapsed) % 60, (int)message->remaining / 60, ((int)message->remaining) % 60, (int)(message->elapsed + message->remaining) / 60, (int)(message->elapsed + message->remaining) % 60);
 	update_panels();
	doupdate ();
}

__inline__ void
clear_play_info (void)
{
	//TODO MOET NOG ANDERS my_mvwnclear (playback->win, 1, 1, playback->width - 2);
	update_panels ();
	if (conf->c_flags & C_FIX_BORDERS)
		redrawwin (playback->win);
}

static void
init_info (Window * window)
{
	WINDOW * win = window->win;
	my_mvwaddstr (win, 1 + window->yoffset, 2, colors[INFO_TEXT], "Title :");
	my_mvwaddstr (win, 2 + window->yoffset, 2, colors[INFO_TEXT], "Artist:");
	my_mvwaddstr (win, 3 + window->yoffset, 2, colors[INFO_TEXT], "Album :");
	if (conf->c_flags & C_USE_GENRE) 
		my_mvwaddstr (win, 4 + window->yoffset, 2, colors[INFO_TEXT], "Genre :");
	
	update_panels ();
	doupdate ();
}
