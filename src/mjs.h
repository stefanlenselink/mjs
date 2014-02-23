#ifndef _mjs_h
#define _mjs_h

#include "defs.h"
#include "config/config.h"

extern Config * conf; //Initialized in config/config.c

void bailout(int sig);

#endif /* _mjs_h */
