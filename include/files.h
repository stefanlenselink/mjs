#ifndef _files_h
#define _files_h

#ifndef _struct_h
#include "struct.h"
#endif

flist	*read_mp3_list(wlist *);
wlist	*read_mp3_list_file(wlist *, char *);
int	write_mp3_list_file(wlist *, char *);
wlist	*sort_songs(wlist *);
wlist	*sort_search(wlist *);
flist	*delete_file(flist *);
flist	*next_valid(flist *, int);

#endif /* _files_h */
