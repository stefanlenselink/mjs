#ifndef _mms_h
#define _mms_h

#ifndef _struct_h
#include "struct.h"
#endif

void	bailout(int);
void	unsuspend(int);
void	process_return(wlist *);
int	update_menu(Input *);
void	update_status(void);
void	show_playinfo(mpgreturn *);
int	dummy_update(Window *);
void	ask_question(char *, char *, char *, Window (*)(Window *));

__inline__ void	clear_play_info();

#endif /* _struct_h */
