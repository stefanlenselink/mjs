#ifndef _mms_h
#define _mms_h

#include "engine/mpgcontrol.h"
#include "gui/window.h"

void	bailout(int);
int	do_search(Input *);
int 	do_save(Input *);
void	unsuspend(int);
void	process_return(wlist *, int);
int	update_menu(Input *);
void	update_status(void);
void	show_playinfo(mpgreturn *);
void	ask_question(char *, char *, char *, Window (*)(Window *));
void    refresh_window(int);
__inline__ void	clear_play_info();

#endif /* _struct_h */
