#ifndef _window_play_h
#define _window_play_h

#include "window.h"
#include "config/config.h"

Window * window_play_init ( Config * conf );
void window_play_update ( void );
void window_play_activate ( void );
void window_play_deactivate ( void );
void window_play_shutdown ( void );
void window_play_notify_title_changed(void);

#endif /* _window_play_h */
