#ifndef _files_h
#define _files_h

#ifndef _struct_h
#include "struct.h"
#endif

flist	*read_mp3_list(wlist *);
wlist	*sort_songs(wlist *);
flist	*delete_file(Window *, flist *);
flist	*next_valid(Window *, flist *, int);

#endif /* _files_h */
