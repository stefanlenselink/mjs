#ifndef _defs_h
#define _defs_h

/* Max length of any field */
#define MAXLEN		500

#define PROMPT_SIZE	40
#define BUFFER_SIZE	1024
#define BIG_BUFFER_SIZE	4096

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

enum ATTRIBS {
	ACTIVE,
	INACTIVE,
	SELECTED,
	UNSELECTED,
	DIRECTORY,
	TITLE,
	SCROLL,
	SCROLLBAR,
	PLAYLIST,
	PLAYING,
	SEL_PLAYING,
	FILE_BACK,
	INFO_BACK,
	PLAY_BACK,
	MENU_BACK,
	MENU_TEXT
};

#endif /* _defs_h */
