#include "defs.h"
#include "mjs.h"
#include "controller/controller.h"
#include "controller/keyboard_controller.h"
#include "gui/gui.h"
#include "gui/window_menubar.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "config.h"
#include "log.h"

#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>



Config *conf;
static struct sigaction handler;
songdata * mp3list, *playlist;
struct itimerval rttimer;
struct itimerval old_rttimer;

//Threads

pthread_t serial_thread;
//pthread_t thread;

/*
 * some internal functions
 */

static void timer_handler(int signum);

int clock_count = 0;

static void timer_handler(int signum) {
	gui_update_play_time();
	if (clock_count == 60) //Every 30 sec?
	{
		window_menubar_update();
		clock_count = 0;
	} else {
		clock_count++;
		keyboard_controller_check_timeout();
	}
}

int main(int argc, char *argv[]) {
	int serial_attached;
	srand(time(NULL));

	memset(&handler, 0, sizeof(struct sigaction));

	/* Ignore the SIGQUIT signals a.k.a pressing Prt Scr */
	handler.sa_handler = SIG_IGN;
	handler.sa_flags = 0;
	sigaction(SIGQUIT, &handler, NULL);

	handler.sa_handler = (SIGHANDLER) bailout;
	sigaction(SIGINT, &handler, NULL);
	handler.sa_handler = (SIGHANDLER) unsuspend;
	handler.sa_flags = SA_RESTART;
	sigaction(SIGCONT, &handler, NULL);

	/*

	 obsolete by new engine system?

	 handler.sa_handler = (SIGHANDLER) restart_mpg_child;
	 handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	 sigaction (SIGCHLD, &handler, NULL);*/

	/*Uitgezet door Bug tijdens de Owee*/

	signal(SIGALRM, timer_handler);
	rttimer.it_value.tv_sec = 0; /* A signal will be sent 250 Milisecond*/
	rttimer.it_value.tv_usec = 500000; /*  from when this is called */
	rttimer.it_interval.tv_sec = 0; /* If the timer is not reset, the */
	rttimer.it_interval.tv_usec = 500000; /*  signal will be sent every 250 Milisecond */

	//Make sure COLS and LINES are set
	if (!initscr())
		bailout(1);

	/**
	 * Do ALL the inits here
	 */
	log_init();
	conf = config_init();
	engine_init(conf);
	mp3list = songdata_init(conf, conf->colors);
	playlist = controller_init(conf);
	gui_init(conf, conf->colors, mp3list, playlist);
	setitimer(ITIMER_REAL, &rttimer, &old_rttimer);
	engine_jump_to("/pub/mp3/.bin/intro.mp3");
	//log_debug("MJS Started!!");

	struct timespec wait;
	wait.tv_sec = 0;
	wait.tv_nsec = 50000000;

	for (;;) {
//		if (serial_attached)
//			serial_poll();
		//poll_keyboard();
//		nanosleep(&wait, NULL);
		sleep(1);
	}
	bailout(-1);
}

void bailout(int sig) {
	/*
	 * Lets get the hell out of here!
	 */
	handler.sa_handler = SIG_IGN;
	handler.sa_flags = 0;
	sigaction(SIGINT, &handler, NULL);

	switch (sig) {
	case 0:
		/*		if (((conf->c_flags & C_P_SAVE_EXIT) > 0) & (play->contents.list->head != NULL)){
		 songdata_save_playlist (play->contents.list, "/home/mvgalen/.previous_playlist.mjs");
		 }
		 if (play->contents.list) {
		 songdata_clear (play->contents.list);
		 free (play->contents.list);
		 }
		 if (files->contents.list){
		 songdata_clear (files->contents.list);
		 free (files->contents.list);
		 }*///TODO anders oplossen

		//TODO class shutdown all function
		break;
	case 1:
		fprintf(stderr, "\n\nmjs:error: Windows are not loaded\n\n\n");
		break;
	default:
		fprintf(stderr, "\n\nmjs:error: unknown\n\n\n");
		break;
	}
	fprintf(stdout, "\n\n MP3 Jukebox System (mjs) v%s\n", VERSION);
	fprintf(stdout, " Copyright (C) 2008\n");
	fprintf(stdout,
			" This program is free software; you can redistribute it and/or modify it\n");
	fprintf(stdout,
			" under the terms of the GNU General Public License as published by the Free\n");
	fprintf(stdout, " Software Foundation; version 2 of the License.\n\n");
	fprintf(
			stdout,
			" This program is distributed in the hope that it will be useful, but WITHOUT\n");
	fprintf(stdout,
			" ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n");
	fprintf(stdout, " FITNESS FOR A PARTICULAR PURPOSE. \n\n");
	fprintf(stdout, " See the GNU GPL (see LICENSE file) for more details.\n");
	fprintf(stdout, " \n");

	rttimer.it_value.tv_sec = 0; /* A signal will be sent 250 Milisecond*/
	rttimer.it_value.tv_usec = 0; /*  from when this is called */
	rttimer.it_interval.tv_sec = 0; /* If the timer is not reset, the */
	rttimer.it_interval.tv_usec = 0; /*  signal will be sent every 250 Milisecond */

	setitimer(ITIMER_REAL, &rttimer, &old_rttimer);
	engine_shutdown();
	songdata_shutdown();
	controller_shutdown();
	gui_shutdown();
	config_shutdown();
	log_shutdown();
	exit(sig);
}

void unsuspend(int sig) {
	wrefresh(curscr);
	curs_set(0);
}
