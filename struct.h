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
	short bitrate;
	int frequency;
	int where;
	time_t length;
	char *filename;
	char *path;
	char *title;
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
	int where;                         /* what position in the list are we?    */
} wlist;

typedef struct _input {
	WINDOW *win;                       /* What window is the inputline in?     */
	int len;                           /* Length of input buffer               */
	int plen;                          /* Length of the input prompt           */
	int pos;                           /* Cursor position inside input buffer  */
	int flen;                          /* Input field length                   */
	int fpos;                          /* Input field position                 */
	int (*parse)(struct _input *);     /* Function that parses input           */
	int (*update)(struct _input *);    /* Function to call to update the line  */
	int (*finish)(struct _input *);    /* Function to call when input is over  */
	char prompt[PROMPT_SIZE+1];        /* Prompt for the input line            */
	char *anchor;                      /* For wrapped lines, ptr to where the  */
                                     /* "anchor" for our wrapped text starts */
	char buf[BUFFER_SIZE+1];           /* The input line itself                */
} Input;

typedef struct _win {
	WINDOW *win;                       /* the actual ncurses window            */
	Input *inputline;                  /* the input stuff                      */
	int (*input) (struct _win *);      /* Function to parse input              */
	int (*update) (struct _win *);     /* Updates the window                   */
	void (*activate)
		(WINDOW *, const u_char *);      /* activate this window                 */
 	void (*deactivate)
 		(WINDOW *, const u_char *);      /* deactivate the window                */
	short int flags;                   /* various info about the window        */
	union {
		wlist *list;                     /* if the window has contents, use this */
		flist *play;                     /* otherwise just this                  */
	} contents;
	const u_char *title;               /* the window title                     */
	struct _win *next;                 /* next window in the cycle             */
	struct _win *prev;                 /* previous window in the cycle         */
} Window;

typedef struct {
	int played;
	int left;
	double elapsed;
	double remaining;
} mpgreturn;
