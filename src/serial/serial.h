#ifndef _serial_h
#define _serial_h

int serial_init(char device[]);
void serial_shutdown();
void serial_poll();
int serial_poll_button();
void serial_set_led(int on);

#endif /*_serial_h */
