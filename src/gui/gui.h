#ifndef _gui_h
#define _gui_h

#include "window.h"

#include "config/config.h"

#include <stdlib.h>

int	 show_list ( Window * );
Window	*move_files_selector ( int );
Window	*move_selector ( Window *, int );
int	 update_info ( Window * );
int update_border(Window *window);
int	 update_title ( Window * );
void	 change_active ( int );
int	 active_win ( Window * );
int	 inactive_win ( Window * );

__inline__ void	 printf_menubar ( char * );

void	 do_scrollbar ( Window * );
__inline__ void clear_info();

void gui_init ( Config *,   int init_colors[], songdata * mp3list, songdata * playlist );
void gui_shutdown ( void );

void poll_keyboard ( void );

int gui_ask_yes_no_question(char *);
int gui_ask_question(char *, char *);

void	gui_update_play_time ( void );
#endif /* _gui_h */
