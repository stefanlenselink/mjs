/* not much here yet */

#ifndef _config_h
#define _config_h

#include <stdlib.h>

/*
 * Ok, this is an enormous pain in the ass. If you know anything about ansi,
 * then you know that there are 8 possible foreground colors and 8 possible
 * background colors. BUT, with the bold flag, the 8 foreground colors
 * become 16. With the reverse video flag combined with bold, the number of
 * background colors on SOME terminals becomes 16. UNFORTUNATELY, the
 * reverse flag doesn't really increase the number of colors -- it flips the
 * possibilities! So if you want 16 colors for the background then you only
 * have 8 choices for the foreground. Ugh.  But all is not lost :)... If you
 * now add in the blinking flag, some terms map this out to allowing the
 * full range of colors in foreground and the background. But it is only
 * SOME terminals. Because of this, I have chosen to stick with 16
 * foreground and 8 background colors. Those of you who know (or figure out)
 * how ncurses stores these attributes internally can easily take advantages
 * of things like this. However, for simplicity I have decided to stick with
 * a 16/8 combo. I'm not using the reverse/blink flag to get the full range
 * of colors.
 */

/* foreground and background colors */

#define BLACK	(0UL)
#define RED		(1UL)
#define GREEN	(2UL)
#define BROWN	(3UL)
#define BLUE	(4UL)
#define MAGENTA	(5UL)
#define CYAN	(6UL)
#define GREY	(7UL)

/* NOTE - These CANNOT be used as background colors! */
#define B_BLACK		(A_BOLD>>11)
#define B_RED		(A_BOLD>>11|1UL)
#define B_GREEN		(A_BOLD>>11|2UL)
#define YELLOW		(A_BOLD>>11|3UL)
#define B_BLUE		(A_BOLD>>11|4UL)
#define B_MAGENTA	(A_BOLD>>11|5UL)
#define B_CYAN		(A_BOLD>>11|6UL)
#define WHITE		(A_BOLD>>11|7UL)



typedef struct _config {
  char mpgpath[256];
  char mp3path[256];
  char statefile[256];
  char logfile[256];
  char resultsfile[256];
  char playlistpath[256];
  char bottomtext[256];
  char output[256];
  char snd_system[256];
  u_int16_t c_flags;
#define C_PADVANCE 	0x0001
#define C_FADVANCE 	0x0002
#define C_MONO	   	0x0004
#define C_LOOP     	0x0008
#define C_P_TO_F 	0x0010
#define C_FIX_BORDERS	0x0020
#define	C_ALLOW_P_SAVE	0x0040
#define C_TRACK_NUMBERS 0x0080
#define	C_P_SAVE_EXIT	0x0100
#define C_USE_GENRE	0x0200
#define C_USE_REFRESH_INTERVAL 0x0400
  int buffer;
  int jump;
  int refresh_interval;
} Config;

Config * config_init(void);
void config_shutdown(Config *);

#endif /* _config_h */
