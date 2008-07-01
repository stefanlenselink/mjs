#ifndef _list_h
#define _list_h

typedef struct _dirstack {
  struct _dirstack *prev;
  char *fullpath;
  char *filename;
} dirstack;

typedef struct _flist {
  unsigned short flags;
#define F_DIR      	0x01
#define F_PLAYLIST	0x02
#define F_HTTP		0x04
#define F_PAUSED   	0x40
  char *album;
  char *filename;		// filename without path
  char *path;		// path without filename without mp3path
  char *fullpath;		// the fullpath
  char *relpath;
  char *artist;
  char *genre;
  char *title;
  int has_id3;
  int track_id;
  int length;
  struct _flist *next;
  struct _flist *prev;
} flist;

typedef struct {
  char *from;
  flist *head;                       /* ptr to the HEAD of the linked list   */
  flist *tail;                       /* ptr to the TAIL of the linked list   */
  flist *top;                        /* ptr to the element at the window top */
  flist *bottom;                        /* ptr to the element at the window bottom */
  flist *selected;                   /* currently selected file              */
  flist *playing;                    /* currently PLAYING file               */
  int length;                        /* length of the list we are tracking   [0..n]*/
  int where;			   /* what position in the list are we?    if length=0 [0] else [1..length]*/
  int wheretop;			   /* at what position is the top */
  int whereplaying;
  unsigned short flags;
  int startposSelected;
#define F_VIRTUAL      	0x01
} wlist;

void 	wlist_add(wlist *, flist *, flist *);
void 	wlist_del(wlist *, flist *);
void	move_backward(wlist *);
void	move_forward(wlist *);
void 	wlist_clear(wlist *);
void	dirstack_push (const char *, const char *);
void	dirstack_pop (void);
char *	dirstack_fullpath (void);
char *	dirstack_filename (void);
int	dirstack_empty (void);

#endif
