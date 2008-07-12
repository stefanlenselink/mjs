#include "defs.h"
#include "mjs.h"
#include "controller/controller.h"
#include "gui/gui.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "config.h"
#include "gui/inputline.h"
#include "extern.h"

#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
/*
 * intialize the external variables 
 */
Window *files, *info, *play, *active, *menubar;
Window *playback, *question;
Config *conf;
static struct sigaction handler;


/*TODO LastFM LastFMHandshake * lastFMHandshake;*/

/*
 * some internal functions 
 */

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
main (int argc, char *argv[])
{
	
	struct timeval wait1000 = { 0, 1000000 };
	int timeout = 0;
	fd_set fds;


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
    
    /*
    
    obsolete by new engine system?
    
	handler.sa_handler = (SIGHANDLER) restart_mpg_child;
	handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction (SIGCHLD, &handler, NULL);*/
	
    
    
	/*Uitgezet door Bug tijdens de Owee*/
	struct itimerval rttimer;
	struct itimerval old_rttimer;
	
	signal(SIGALRM,timer_handler);
	rttimer.it_value.tv_sec     = 3; /* A signal will be sent 250 Milisecond*/
	rttimer.it_value.tv_usec    = 0; /*  from when this is called */
	rttimer.it_interval.tv_sec  = 3; /* If the timer is not reset, the */
	rttimer.it_interval.tv_usec = 0; /*  signal will be sent every 250 Milisecond */
	
	setitimer (ITIMER_REAL, &rttimer, &old_rttimer);

    /**
     * Do ALL the inits here 
     */
    gui_init(conf, conf->colors);
    conf = config_init();
    controller_init(conf);
    engine_init(conf);
    songdata_init(conf, conf->colors);
    



	if (argc > 1)
		read_mp3_list_array(play->contents.list, argc, argv);
	else if (conf->c_flags & C_P_SAVE_EXIT)
		read_mp3_list_file (play->contents.list, "/home/mvgalen/.previous_playlist.mjs", 1);



/* TODO LastFM    lastFMHandshake = malloc(sizeof(LastFMHandshake));*/
/* TODO LastFM   initLastFM(lastFMHandshake, "L-Tech", "xxx");*/
    
    //Main loop
	for (;;) {
		FD_ZERO (&fds);
		FD_SET (0, &fds);
		if (select (FD_SETSIZE, &fds, NULL, NULL, &wait1000) > 0) {
			if (FD_ISSET (0, &fds)) {
				if (active != menubar)
					info->contents.show = &active->contents.list->selected;
				info->update(info);
				active->input (active);
				timeout = 0;
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

	exit (sig);
}




void
unsuspend (int sig)
{
	wrefresh (curscr);
	curs_set (0);
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

	if (1 /*p_status == STOPPED*/) { //TODO nog aanpassen naar nieuwe engine
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



__inline__ void
clear_play_info (void)
{
	//TODO MOET NOG ANDERS my_mvwnclear (playback->win, 1, 1, playback->width - 2);
	update_panels ();
	if (conf->c_flags & C_FIX_BORDERS)
		redrawwin (playback->win);
}


