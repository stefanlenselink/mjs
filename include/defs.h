#ifndef _defs_h
#define _defs_h

#define SONGFILE ".songs"

/* Max length of any field */
#define MAXLEN		500

#define PROMPT_SIZE	40
#define BUFFER_SIZE	1024
#define BIG_BUFFER_SIZE	4096

/* defines for the mpg123 controller */

#define MPGPATH		"mpg123"

#ifdef GPM_SUPPORT
#define WGETCH 		Gpm_Wgetch
#else
#define WGETCH		wgetch
#endif

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
	TITLE,
	SCROLL,
	SCROLLBAR,
	PLAYING,
	SEL_PLAYING,
	FILE_BACK,
	INFO_BACK,
	PLAY_BACK,
	MENU_BACK,
	MENU_TEXT,
	ARROWS,
	EDIT_BACK,
	EDIT_ACTIVE,
	EDIT_INACTIVE,
	EDIT_PROMPT
};

#endif /* _defs_h */
