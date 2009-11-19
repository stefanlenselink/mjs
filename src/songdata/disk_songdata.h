#ifndef _disk_songdata_h
#define _disk_songdata_h

#include "songdata.h"
#include "config/config.h"

void disk_songdata_read_mp3_list_dir ( songdata *, const char *, int );

void disk_songdata_init(Config * conf);

songdata_song *mp3_info ( const char *, const char *, const char *, int );

#endif
