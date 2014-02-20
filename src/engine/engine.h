#ifndef _engine_h
#define _engine_h

#include "config/config.h"
#include "songdata/songdata.h"


void engine_init ( Config * conf);
void engine_stop ( void );
void engine_jump_to ( char *);
int engine_is_paused(void);
int engine_is_playing(void);
void engine_resume_playback(void);
void engine_pause_playback(void);
void engine_ffwd ( int , int );
void engine_frwd ( int , int );
void engine_shutdown ( void );
int engine_get_elapsed ( void );
int engine_get_remaining ( void );
int engine_get_length ( void );
void engine_load_meta_info(songdata_song *);
int engine_extention_is_supported(char *);

#endif /* _engine_h */
