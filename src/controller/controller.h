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

wlist	*read_playlist ( wlist *, const char * );

wlist * controller_init (Config *);
void controller_shutdown ( void );
void controller_playlist_move_up();
void controller_playlist_move_down();
void controller_process_to_next_song ( void );
void controller_jump_to_song ( flist * );
void controller_check_timeout(void);
void controller_stop(void);

#endif /* _playlist_h */
