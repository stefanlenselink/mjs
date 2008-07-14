#ifndef _playlist_h
#define _playlist_h

#include "songdata/songdata.h"
#include "gui/gui.h"

#include "config/config.h"

#include <signal.h>
#include <errno.h>

#define STOPPED 0
#define PLAYING 1
#define PAUSED 2



void	 play_next_song();
void	 play_prev_song();

int	 playback_title ( Window * );

void	add_to_playlist_recursive ( wlist *, flist *, flist * );
void	add_to_playlist ( wlist *, flist *, flist * );
void	stop_player();
wlist	*randomize_list ( wlist * );
wlist	*read_playlist ( wlist *, const char * );

wlist * controller_init ( Config *, wlist * );
void controller_shutdown ( void );
int read_keyboard ( Window * );



void controller_playlist_move_up();
void controller_playlist_move_down();
void controller_update_whereplaying ( void );
void controller_update_statefile ( void );
void controller_process_to_next_song ( void );
void controller_jump_to_song ( flist * );

#endif /* _playlist_h */
