#define __USE_UNIX98
#define _GNU_SOURCE

#define SONGFILE ".songs"

/* file list bit fields */
#define F_DIR      0x01
#define F_PLAY     0x02
#define F_SELECTED 0x04
#define F_PAUSED   0x08

/* window bit fields */
#define W_ACTIVE   0x01              /* is the window active?                */
#define W_LIST     0x02              /* is it a list, or just 'contents'     */
#define W_RDONLY   0x04              /* is it read-only, ie can we modify it */

/* Max length of any field */
#define MAXLEN 500

#define PROMPT_SIZE 40
#define BUFFER_SIZE 1024
#define BIG_BUFFER_SIZE 4096

/* defines for the mpg123 controller */

/* Set this to the right path! */
/* #define MPGPATH "/usr/local/bin/mpg123" */

#define QUIT 1
#define LOAD 2
#define STOP 3
#define PAUSE 4
#define JUMP 5

#if defined __GLIBC__ && __GLIBC__ >= 2
#define SIGHANDLER sighandler_t
#else
#define SIGHANDLER void *
#endif

#ifndef MPGPATH
#error "define MPGPATH in defs.h!"
#endif
