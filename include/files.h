#ifndef _files_h
#define _files_h

#ifndef _struct_h
#include "struct.h"
#endif

void	read_mp3_list(wlist *);
void	read_mp3_list_file(wlist *, char *, int);
int	write_mp3_list_file(wlist *, char *);
wlist	*sort_songs(wlist *);
wlist	*sort_search(wlist *);
flist	*next_valid(wlist *, flist *, int);

#endif /* _files_h */
