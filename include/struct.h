#ifndef _struct_h
#define _struct_h

#ifndef _defs_h
#include "defs.h"
#endif  /* _defs_h */

#define STOPPED 0
#define PLAYING 1
#define PAUSED 2


typedef struct {
	char tag[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	char comment[30];
	unsigned char genre;
} ID3tag;

typedef struct {
	char tag[3];
	u_int8_t major_version;
	u_int8_t minor_version;
	u_int8_t flags;
#define UNSYNC 0x100
#define EXTENDED_HEADER 0x80
#define EXPERIMENTAL 0x40
	int32_t	size;
} ID3v2Header;

typedef struct {
	char title[31];
	char artist[31];
	char album[31];
	char year[5];
	char comment[31];
	char genre[20];
} my_tag;

typedef struct {
	int IDex;
	int ID;
	int layer;
	int protection_bit;
	int bitrate_index;
	int sampling_frequency;
	int padding_bit;
	int private_bit;
	int mode;
	int mode_extension;
	int copyright;
	int original;
	int emphasis;
	int stereo;
	int jsbound;
	int sblimit;
	int true_layer;
	int framesize;
} AUDIO_HEADER;

typedef struct _flist {
	unsigned short flags;
#define F_DIR      	0x01
#define F_PLAY     	0x02
#define F_SELECTED 	0x04
#define F_PAUSED   	0x08
#define F_SEARCHDIR	0x10	
#define F_PLAYLIST	0x20
	int where;
	time_t length;
	char *album;
	char *filename;
	char *path;
	char *fullpath;
	char *artist;
	struct _flist *next;
	struct _flist *prev;
} flist;

typedef struct {
	flist *head;                       /* ptr to the HEAD of the linked list   */
	flist *tail;                       /* ptr to the TAIL of the linked list   */
	flist *top;                        /* ptr to the element at the window top */
	flist *selected;                   /* currently selected file              */
	flist *playing;                    /* currently PLAYING file               */
	int length;                        /* length of the list we are tracking   */
	int where;			   /* what position in the list are we?    */
	int wheretop;			   /* at what position is the top */
	time_t duration;
} wlist;

typedef struct _input {
	WINDOW *win;                       /* The window where we are              */
	PANEL *panel;                      /* The panel where we are               */
	int x;                             /* x-coord for the start of our input   */
	int y;                             /* y-coord for the start of our input   */
	int len;                           /* Length of input buffer               */
	int plen;                          /* Length of the input prompt           */
	int pos;                           /* Cursor position inside input buffer  */
	int flen;                          /* Input field length                   */
	int fpos;                          /* Input field position                 */
	int (*parse)
	    (struct _input *, int, int);   /* Function that parses input           */
	int (*update)(struct _input *);    /* Function to call to update the line  */
	int (*finish)(struct _input *);    /* Function to call when input is over  */
	int (*complete)(struct _input *);  /* Function for tab completion          */
	char prompt[PROMPT_SIZE+1];        /* Prompt for the input line            */
	char *anchor;                      /* For wrapped lines, ptr to where the  */
					   /* "anchor" for our wrapped text starts */
	char buf[BUFFER_SIZE+1];           /* The input line itself                */
} Input;

typedef struct _win {
	WINDOW *win;                       /* the actual ncurses window            */
	PANEL *panel;                      /* the panel, duh                       */
	int width;                         /* window/panel width                   */
	int height;                        /* window/panel height                  */
	int x;                             /* x-coord for the upper-left corner    */
	int y;                             /* y-coord for the upper-left corner    */
	Input *inputline;                  /* the input stuff                      */
	int (*input) (struct _win *);      /* Function to parse input              */
	int (*update) (struct _win *);     /* Updates the window                   */
	int (*activate) (struct _win *);   /* activate this window                 */
	int (*deactivate) (struct _win *); /* deactivate the window                */
	short int flags;                   /* various info about the window        */
#define W_ACTIVE   0x01              /* is the window active?                */
#define W_LIST     0x02              /* is it a list, or just 'contents'     */
#define W_RDONLY   0x04              /* is it read-only, ie can we modify it */
	union {
		wlist *list;               /* if the window has contents, use this */
		flist *play;               /* otherwise just this                  */
	} contents;
	const u_char *title_fmt;           /* format string for window title       */
	const u_char *title_dfl;           /* default (if needed) window title     */
	u_char *title;                     /* the window title                     */
	const u_char *format;              /* format for the text in the window    */
	struct _win *next;                 /* next window in the cycle             */
	struct _win *prev;                 /* previous window in the cycle         */
} Window;

typedef struct _config {
	char mpgpath[256];
	char mp3path[256];
	char statefile[256];
	char logfile[256];
	char resultsfile[256];
	char playlistpath[256];
	char bottomtext[256];
	char output[256];
	u_int16_t c_flags;
#define C_PADVANCE 	0x0001
#define C_FADVANCE 	0x0002
#define C_LOOP     	0x0008
#define C_P_TO_F 	0x0010
#define C_MONO	   	0x0004
#define C_SHOW_P_LENGTH	0x0020
#define C_ALT_SCROLL	0x0040
#define	C_ALLOW_P_SAVE	0x0080
#define C_TRACK_NUMBERS 0x0100
	int buffer;
	int jump;
} Config;

typedef struct {
	int played;
	int left;
	double elapsed;
	double remaining;
} mpgreturn;

/* this struct holds all the info to construct a question/answer input dialog */

typedef struct {
	char **prompts;                 /* array of prompts for our edit box */
	char **inputs;                  /* array of default values           */
	int num_prompts;                /* total # of prompts/defaults       */
	int (*finish) (char **, int);   /* function to parse the final input */
	



} Inputbox;

#endif /* _struct_h */
