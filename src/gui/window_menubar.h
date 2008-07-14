#ifndef _window_menubar_h
#define _window_menubar_h

#include "window.h"
#include "config/config.h"

Window * window_menubar_init ( Config * conf );
void window_menubar_update ( void );
void window_menubar_activate ( void );
void window_menubar_deactivate ( void );
void window_menubar_shutdown ( void );

#endif /* _window_menubar_h */
