/***************************************************************************
 *   Copyright (C) 2008 by Hidde Boomsma                                   *
 *   hidde@bolkhuis.nl                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "plugin/plugin.h"
#include "controller/controller.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "config/config.h"

extern Config * conf;

static int fd;
static pthread_t thread;

void f12automaat_set_led(int on) {
	int status;
	ioctl(fd, TIOCMGET, &status);
	if (on)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;
	ioctl(fd, TIOCMSET, &status);
}

int f12automaat_poll_button(void) {
	int state;
	ioctl(fd, TIOCMGET, &state); /* read interface */
	if (state & TIOCM_CTS) {
		return (1);
	} else {
		return (0);
	}
}

void f12automaat_poll(void) {
	static int prev_state = 0;
	int next_state = f12automaat_poll_button();
	if (next_state && next_state != prev_state) {
		controller_next();
		f12automaat_set_led(0);
		sleep(1);
		f12automaat_set_led(1);
		sleep(1);
		f12automaat_set_led(0);
		sleep(1);
		f12automaat_set_led(1);
		sleep(1);
		f12automaat_set_led(0);
		sleep(1);
		f12automaat_set_led(1);
		sleep(1);
		f12automaat_set_led(0);
		sleep(1);
		f12automaat_set_led(1);
	}
	prev_state = next_state;
}

void *f12automaat_thread(void * arg){
	log_debug("MJS f12automaat started!\n");
	
	while(1){
		f12automaat_poll();
		usleep(1000);
	}
}

void f12automaat_init(void) {
	if (conf->serial_device == NULL) {
		return;
	}
	fd = open(conf->serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd != -1) {
		fcntl(fd, F_SETFL, 0);
		//Start the thread
		pthread_create(&thread, NULL, f12automaat_thread, NULL);
	}
}

void f12automaat_shutdown(void) {
	close(fd);
}

PLUGIN_REGISTER(f12automaat_init, f12automaat_shutdown)
