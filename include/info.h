#ifndef _info_h
#define _info_h

#ifndef _struct_h
#include "struct.h"
#endif

char	*chop_filename(char**);
char	*strip_track_numbers(char *);
flist 	*mp3_info(const char *, const char *, const char *, int, int);
char 	*resolve_path (const char *);

#endif /* _info_h */
