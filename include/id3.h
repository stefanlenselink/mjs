#ifndef _id3_h
#define _id3_h

#ifndef _struct_h
#include "struct.h"
#endif

void	edit_tag(flist *);
int	id3_input(Input *, int, int);
int	update_edit(Window *);

#endif /* _id3_h */
