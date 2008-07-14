#ifndef _defs_h
#define _defs_h

/* Max length of any field */
#define MAXLEN		500

#define PROMPT_SIZE	40
#define BUFFER_SIZE	1024
#define BIG_BUFFER_SIZE	4096

#define SHUFFLE "Shuffle Playlist ? (y/n)"
#define SEARCHING "Searching...   [                                                   ]"
#define READING   "Reading...     [                                                   ]"
#define CLEARPLAYLIST "Are you sure you want to clear the playlist ? (y/n)"
#define EXITPROGRAM "Are you sure you want to reset this program ? (y/n)"

/* defines for the mpg123 controller */

#define MPGPATH		"mpg123"

#define QUIT		1
#define LOAD		2
#define STOP		3
#define PAUSE		4
#define JUMP		5

#if defined __GLIBC__ && __GLIBC__ >= 2
#define SIGHANDLER	sighandler_t
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define SIGHANDLER	sig_t
#else
#define SIGHANDLER	void *
#endif

/* enumeration for the color array */

#define NUM_COLORS 24

enum ATTRIBS
{
	WIN_ACTIVE,
	WIN_ACTIVE_TITLE,
	WIN_ACTIVE_SCROLL,
	WIN_ACTIVE_SCROLLBAR,
	WIN_INACTIVE,
	WIN_INACTIVE_TITLE,
	WIN_INACTIVE_SCROLL,
	WIN_INACTIVE_SCROLLBAR,
	INFO_TEXT,
	INFO_WINDOW,
	PLAYBACK_TEXT,
	PLAYBACK_WINDOW,
	PLAY_UNSELECTED,
	PLAY_SELECTED,
	PLAY_UNSELECTED_PLAYING,
	PLAY_SELECTED_PLAYING,
	PLAY_WINDOW,
	FILE_UNSELECTED,
	FILE_SELECTED,
	FILE_UNSELECTED_DIRECTORY,
	FILE_SELECTED_DIRECTORY,
	FILE_WINDOW,
	MENU_TEXT,
	MENU_WINDOW
};

#endif /* _defs_h */
