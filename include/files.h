#ifndef _files_h
#define _files_h

#ifndef _struct_h
#include "struct.h"
#endif


void	leave_directory (Window *);
void	enter_directory (Window *, const char *);
void	read_mp3_list(wlist *, const char *, int);
void	read_mp3_list_dir(wlist *, const char *, int);
void	read_mp3_list_file(wlist *, const char *, int);
void 	read_mp3_list_array(wlist *, int, char * []);
int	write_mp3_list_file(wlist *, char *);
wlist	*sort_songs(wlist *);
wlist	*sort_search(wlist *);
__inline__ int check_file (flist *);
flist	*next_valid(wlist *, flist *, int);

#endif /* _files_h */
