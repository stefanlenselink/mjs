#ifndef _list_h
#define _list_h

#ifndef _struct_h
#include "struct.h"
#endif

void 	wlist_add(wlist *, flist *, flist *);
void 	wlist_del(wlist *, flist *);
void    wlist_move(wlist *, wlist *);
void	move_backward(wlist *);
void	move_forward(wlist *);
void 	wlist_clear(wlist *);
void	dirstack_push (const char *, const char *, wlist *);
void	dirstack_pop (void);
char *	dirstack_fullpath (void);
char *	dirstack_filename (void);
wlist *	dirstack_listcache (void);
int	dirstack_empty (void);

#endif
