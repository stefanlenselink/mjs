#ifndef _misc_h
#define _misc_h

#ifndef _struct_h
#include "struct.h"
#endif

void	init_ansi_pair(void);

int	my_waddstr(WINDOW *, u_int32_t, const u_char *);
int	my_mvwaddstr(WINDOW *, int, int, u_int32_t, const u_char *);
int	my_wnaddstr(WINDOW *, u_int32_t, size_t, const u_char *);
int	my_mvwnaddstr(WINDOW *, int, int, u_int32_t, size_t, const u_char *);
int	my_wprintw(WINDOW *, u_int32_t, const u_char *, ...);
int	my_mvwprintw(WINDOW *, int, int, u_int32_t, const u_char *, ...);
int	my_wnprintw(WINDOW *, u_int32_t, int, const u_char *, ...);
int	my_mvwnprintw(WINDOW *, int, int, u_int32_t, int, const u_char *, ...);
int	my_mvwnprintw2(WINDOW *, int, int, u_int32_t, int, const u_char *, ...);

__inline__ void	my_wnclear(WINDOW *, int);
__inline__ void	my_mvwnclear(WINDOW *, int, int, int);

#endif /* _struct_h */
