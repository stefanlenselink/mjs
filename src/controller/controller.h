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





int	 playback_title ( Window * );

void	add_to_playlist_recursive ( wlist *, flist *, flist * );
void	add_to_playlist ( wlist *, flist *, flist * );

wlist	*read_playlist ( wlist *, const char * );


wlist * controller_init (Config *);
void controller_shutdown ( void );
void controller_playlist_move_up();
void controller_playlist_move_down();
char * controller_process_to_next_song ( void );
void controller_jump_to_song ( flist * );
void controller_play_pause(void);
void controller_check_timeout(void);
void controller_stop(void);
int controller_has_next_song( void );
void	 controller_next();
void	 controller_prev();
void controller_clear_playlist();

#endif /* _playlist_h */
