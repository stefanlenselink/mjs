#ifndef _playlist_h
#define _playlist_h

#ifndef _struct_h
#include "struct.h"
#endif

void	 play_next_song(void);

int	 do_read_playlist(Input *);
int	 do_save_playlist(Input *);
int	 write_playlist(wlist *, const char *);
int	 jump_to_song(flist *);
int	 playback_title(Window *);

wlist	*add_to_playlist(wlist *, flist *);
wlist	*stop_player(wlist *);
wlist	*pause_player(wlist *);
wlist	*jump_forward(wlist *);
wlist	*jump_backward(wlist *);
wlist	*randomize_list(wlist *);
wlist	*read_playlist(wlist *, const char *);

#endif /* _playlist_h */
