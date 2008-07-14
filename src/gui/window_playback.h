#ifndef _window_playback_h
#define _window_playback_h

#include "window.h"
#include "config/config.h"

Window * window_playback_init ( Config * conf );
void window_playback_update ( void );
void window_playback_activate ( void );
void window_playback_deactivate ( void );
void window_playback_shutdown ( void );

#endif /* _window_playback_h */
