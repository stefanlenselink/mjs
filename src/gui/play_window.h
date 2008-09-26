#ifndef _play_window_h
#define _play_window_h

#include "window.h"

Window * window_play_init(void);
void window_play_update(void);
void window_play_activate(void);
void window_play_deactivate(void);
void window_play_shutdown(void);

#endif /* _play_window_h */