#ifndef _mjs_h
#define _mjs_h

#include "defs.h"
#include "config.h"
#include "config/config.h"
#include "songdata/songdata.h"

Config *conf;
songdata *mp3list;
songdata *playlist;

void bailout(int sig);

#endif /* _mjs_h */
