#ifndef _window_files_h
#define _window_files_h

#include "window.h"
#include "config/config.h"

Window * window_files_init(Config * conf);
void window_files_update(void);
void window_files_activate(void);
void window_files_deactivate(void);
void window_files_shutdown(void);

#endif /* _window_files_h */
