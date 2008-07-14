#ifndef _window_info_h
#define _window_info_h

#include "window.h"
#include "config/config.h"

Window * window_info_init ( Config * conf );
void window_info_update ( void );
void window_info_activate ( void );
void window_info_deactivate ( void );
void window_info_shutdown ( void );

#endif /* _window_info_h */
