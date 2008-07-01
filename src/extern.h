/* external stuff */

#ifndef _extern_h
#define _extern_h

#include "config/config.h"
#include "gui/window.h"
#include <unistd.h>
#include <stdlib.h>

extern Window		*files, *info, *play, *active, *menubar;
extern Window		*old_active;
extern Window		*playback, *question;
extern Config		*conf;
extern pid_t		 pid; /* pid of the player */
extern int		 p_status; /* player status */

/* colors */
extern u_int32_t	 colors[];

#endif /* _extern_h */
