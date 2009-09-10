#ifndef _mysql_songdata_h
#define _mysql_songdata_h

#include "songdata.h"
#include "config/config.h"

void mysql_songdata_read_mp3_list_dir ( songdata *, const char *, int );

void mysql_songdata_init(Config * conf);
void mysql_songdata_shutdown();

#endif