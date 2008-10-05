#ifndef _misc_h
#define _misc_h

#include <ncurses.h>
#include <stdlib.h>

void	init_ansi_pair ( void );

int	my_waddstr ( WINDOW *, int, const char * );
int	my_mvwaddstr ( WINDOW *, int, int, int, const char * );
// int	my_wnaddstr(WINDOW *, int, size_t, const char *);
int	my_mvwnaddstr ( WINDOW *, int, int, int, size_t, const char *, int );
// int	my_wprintw(WINDOW *, int, const char *, ...);
int	my_mvwprintw ( WINDOW *, int, int, int, const char *, ... );
int	my_wnprintw ( WINDOW *, int, int, const char *, ... );
int	my_mvwnprintw ( WINDOW *, int, int, int, int, const char *, ... );
int	my_mvwnprintw2 ( WINDOW *, int, int, int, int, const char *, ... );

__inline__ void	my_wnclear ( WINDOW *, int );
__inline__ void	my_mvwnclear ( WINDOW *, int, int, int );

#endif /* _struct_h */
