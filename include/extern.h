/* external stuff */

#ifndef _extern_h
#define _extern_h

#ifndef _struct_h
#include "struct.h"
#endif

extern Window		*files, *info, *play, *active, *menubar, *id3box;
extern Window		*old_active;
extern Window		*playback, *question;
extern Config		*conf;
extern pid_t		 pid; /* pid of the player */
extern int		 p_status; /* player status */
extern char		 version_str[128];
extern char		*Genres[255];

/* colors */
extern u_int32_t	 colors[];

#endif /* _extern_h */
