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

#include "serial_controller.h"
#include "controller/controller.h"
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

static int fd;
pthread_t serial_thread;

void serial_controller_init(Config * cnf) {
	if (cnf->serial_device == NULL) {
		return;
	}
	fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd != -1) {
		fcntl(fd, F_SETFL, 0);
		//Start Serial thread
		pthread_create(&serial_thread, NULL, serial_controller_thread, NULL);
	}
}

void serial_controller_shutdown(void) {
	close(fd);
}

void serial_poll(void) {
	static int prev_state = 0;
	int next_state = serial_poll_button();
	if (next_state && next_state != prev_state) {
		controller_next();
		serial_set_led(0);
		sleep(1);
		serial_set_led(1);
		sleep(1);
		serial_set_led(0);
		sleep(1);
		serial_set_led(1);
		sleep(1);
		serial_set_led(0);
		sleep(1);
		serial_set_led(1);
		sleep(1);
		serial_set_led(0);
		sleep(1);
		serial_set_led(1);
	}
	prev_state = next_state;
}

int serial_poll_button(void) {
	int state;
	ioctl(fd, TIOCMGET, &state); /* read interface */
	if (state & TIOCM_CTS) {
		return (1);
	} else {
		return (0);
	}
}

void serial_set_led(int on) {
	int status;
	ioctl(fd, TIOCMGET, &status);
	if (on)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;
	ioctl(fd, TIOCMSET, &status);
}

void * serial_controller_thread(void * arg){
	while(1){
		serial_poll();
		sleep(1);
	}
}
