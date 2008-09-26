#ifndef _gui_h
#define _gui_h

#include "window.h"

#include "config/config.h"

#include <stdlib.h>


int	 show_list ( Window * );
Window	*move_files_selector ( int );
Window	*move_selector ( Window *, int );
int	 update_info ( Window * );
int	 update_title ( Window * );
void	 change_active ( int );
int	 active_win ( Window * );
int	 inactive_win ( Window * );
int	 std_menubar ( Window * );
__inline__ void	 printf_menubar ( char * );
int	 clear_menubar ( Window * );
void	 do_scrollbar ( Window * );
__inline__ void clear_info();

void gui_init ( Config *,   u_int32_t init_colors[], wlist * mp3list, wlist * playlist );
void gui_shutdown ( void );

void poll_keyboard ( void );

int gui_ask_question(char *);

int	update_menu ( Input * );
void	show_playinfo ( void );
#endif /* _gui_h */
