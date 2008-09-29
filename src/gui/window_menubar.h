#ifndef _window_menubar_h
#define _window_menubar_h

#include "window.h"
#include "config/config.h"

Window * window_menubar_init ( Config * conf );
void window_menubar_update ( void );
void window_menubar_activate ( void );
void window_menubar_deactivate ( void );
void window_menubar_shutdown ( void );
int	 window_menubar_standard ( Window * );
int	 window_menubar_clear ( Window * );
void window_menubar_progress_bar_animate(void);
void window_menubar_progress_bar_init(char *);
void window_menubar_progress_bar_progress(int);
void window_menubar_progress_bar_remove(void);

#endif /* _window_menubar_h */
