#ifndef _window_h
#define _window_h

#ifndef _struct_h
#include "struct.h"
#endif

int	 show_list(Window *);
Window	*move_selector(Window *, int);
int	 update_info(Window *);
int	 update_title(Window *);
void	 change_active(Window *);
int	 active_win(Window *);
int	 inactive_win(Window *);
void	 do_scrollbar(Window *);
int	 inactive_edit(Window *);

#endif /* _window_h */
