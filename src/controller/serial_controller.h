#ifndef _serial_controller_h
#define _serial_controller_h

#include "config/config.h"

void serial_controller_init(Config *);
void serial_controller_shutdown();
void serial_poll();
int serial_poll_button();
void serial_set_led(int on);
void * serial_controller_thread(void * arg);

#endif /*_serial_controller_h */
