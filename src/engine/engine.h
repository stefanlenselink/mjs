#ifndef _engine_h
#define _engine_h

#include "songdata/songdata.h"

//Utility
void engine_init ( void );
void engine_shutdown ( void );
int engine_extention_is_supported( char * );

//Engine playback controls:
void engine_stop ( void );
void engine_jump_to ( char * );
void engine_resume_playback( void );
void engine_pause_playback( void );
void engine_ffwd ( int , int );
void engine_frwd ( int , int );

//Engine state:
int engine_is_playing( void );
int engine_is_paused( void );
int engine_get_elapsed ( void );
int engine_get_remaining ( void );
int engine_get_length ( void );

//Metadata
void engine_load_meta_info( songdata_song * );


#endif /* _engine_h */
