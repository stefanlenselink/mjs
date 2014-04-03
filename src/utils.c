#include "utils.h"
#include "controller/controller.h"
#include "gui/gui.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "config/config.h"
#include "log.h"
//#include "plugin/plugin.h"

#include <string.h>
#include <signal.h>
#include <errno.h>

#include <string.h>
#include <ctype.h>


static struct sigaction handler;

/**
 * Trims whitespace from string at s in place, moves start of string back to s.
 */
char * strtrim(char * s)
{
	char * front = s - 1; //Decrement for cleaner while loop.
	char * end = NULL;
	size_t len = 0;
	size_t newlen = 0;

	if( s == NULL )
		return NULL;

	if( *s == '\0' )
		return s;

	len = strlen(s);
	end = s + len;

	//Move front and end pointers forward/backward if whitespace found.
	while( isspace(*(++front)) )
	while( isspace(*(--end)) && end != front )


	if( s + len - 1 != end)
	{
		*(end + 1) = '\0';
	}
	else if( front != s && end == front )
	{
		*s = '\0';
		return s; //No need to copy back to front
	}


	//Copy resulting string back to start at s
	newlen = strlen(front);
	memmove( s, front, newlen+1);

	return s;
}

void utils_init_sig_handlers( void ){
	memset(&handler, 0, sizeof(struct sigaction));

	/* Ignore the SIGQUIT signals a.k.a pressing Prt Scr */
	handler.sa_handler = SIG_IGN;
	handler.sa_flags = 0;
	sigaction(SIGQUIT, &handler, NULL);

	handler.sa_handler = (SIGHANDLER) bailout;
	sigaction(SIGINT, &handler, NULL);

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

	//plugin_shutdown();
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
