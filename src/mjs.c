#include "defs.h"
#include "mjs.h"
#include "controller/controller.h"
#include "gui/gui.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "config/config.h"
#include "log.h"
#include "plugin/plugin.h"

#include <string.h>
#include <signal.h>
#include <errno.h>

#include <time.h>
#include <locale.h>

static struct sigaction handler;

int main(int argc, char *argv[]) {
	srand(time(NULL));

	memset(&handler, 0, sizeof(struct sigaction));

	/* Ignore the SIGQUIT signals a.k.a pressing Prt Scr */
	handler.sa_handler = SIG_IGN;
	handler.sa_flags = 0;
	sigaction(SIGQUIT, &handler, NULL);

	handler.sa_handler = (SIGHANDLER) bailout;
	sigaction(SIGINT, &handler, NULL);

	//Prevent UTF-8 BUGS
	setlocale(LC_ALL,"");

	//Make sure COLS and LINES are set
	// TODO move to gui_init
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
	gui_init();	
	plugin_init();
	
	//engine_jump_to("/home/hidde/Music/intro.mp3");
	log_debug("MJS started!!\n");
	gui_loop();
	
	bailout(-1);

	return 0;
}

void bailout(int sig) {	
	/*
	 * Lets get the hell out of here!
	 */
	handler.sa_handler = SIG_IGN;
	handler.sa_flags = 0;
	sigaction(SIGINT, &handler, NULL);
	sigaction(SIGALRM, &handler, NULL);

	switch (sig) {
	default:
		break;
	}

	plugin_shutdown();
	gui_shutdown();	
	engine_shutdown();
	songdata_shutdown();
	controller_shutdown();
	config_shutdown();
	log_shutdown();

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

	exit(sig);
}
