#ifndef _playlist_h
#define _playlist_h

#ifndef _struct_h
#include "struct.h"
#endif

void	 calculate_duration(flist *);
void	 play_next_song(void);
void	 free_playlist(wlist *);

int	 jump_to_song(flist *);
int	 playback_title(Window *);

void	add_to_playlist(wlist *, flist *, flist *);
wlist	*stop_player(wlist *);
wlist	*pause_player(wlist *);
wlist	*resume_player(wlist *);
wlist	*jump_forward(wlist *);
wlist	*jump_backward(wlist *);
wlist	*move_backward(wlist *);
wlist	*move_forward(wlist *);
wlist	*randomize_list(wlist *);
wlist	*read_playlist(wlist *, const char *);

#endif /* _playlist_h */
