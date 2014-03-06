#ifndef _playlist_h
#define _playlist_h

#include "songdata/songdata.h"
#include "gui/gui.h"

#include <signal.h>
#include <errno.h>

#define STOPPED 0
#define PLAYING 1
#define PAUSED 2

//Utility
void controller_init( void );
void controller_shutdown ( void );
char * controller_process_to_next_song ( void );
void controller_exit( void );

//Playlist management.
void add_to_playlist_recursive( songdata *, songdata_song *, songdata_song * );
void add_to_playlist( songdata *, songdata_song *, songdata_song * );
void controller_playlist_move_up( void );
void controller_playlist_move_down( void );
void controller_clear_playlist( void );
void controller_shuffle_playlist( void );
void controller_save_playlist( char * );

//Search
void controller_reload_search_results( void );
void controller_search(char *);

//Playback management.
void controller_jump_to_song ( songdata_song * );
void controller_play_pause( void );
void controller_stop( void );
void controller_next( void );
void controller_prev( void );

#endif /* _playlist_h */
