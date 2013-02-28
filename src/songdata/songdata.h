#ifndef _songdata_h
#define _songdata_h

#include "config/config.h"
#include <unistd.h>
#include <stdlib.h>
typedef struct _dirstack
{
	struct _dirstack *prev;
	char *fullpath;
	char *filename;
} dirstack;

typedef struct _songdata_song
{
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
	char *tag;
	int has_id3;
	int track_id;
	int length;
    int catalog_id;
	struct _songdata_song *next;
	struct _songdata_song *prev;
} songdata_song;

typedef struct
{
	char *from;
	songdata_song *head;                       /* ptr to the HEAD of the linked list   */
	songdata_song *tail;                       /* ptr to the TAIL of the linked list   */
	songdata_song *top;                        /* ptr to the element at the window top */
	songdata_song *bottom;                        /* ptr to the element at the window bottom */
	songdata_song *selected;                   /* currently selected file              */
	songdata_song *playing;                    /* currently PLAYING file               */
	int length;                        /* length of the list we are tracking   [0..n]*/
	int where;			   /* what position in the list are we?    if length=0 [0] else [1..length]*/
	int wheretop;			   /* at what position is the top */
	int whereplaying;
	unsigned short flags;
	int startposSelected;
#define F_VIRTUAL      	0x01
} songdata;

#define L_APPEND      	0
#define L_NEW		1
#define L_SEARCH	2



void	songdata_read_mp3_list ( songdata *, const char *, int );


int	songdata_save_playlist ( songdata *, char * );


int songdata_check_file ( songdata_song * );
songdata_song	*songdata_next_valid ( songdata *, songdata_song *, int );

void 	songdata_add ( songdata *, songdata_song *, songdata_song * );
void 	songdata_del ( songdata *, songdata_song * );
void 	songdata_clear ( songdata * );
void	dirstack_push ( const char *, const char * );
void	dirstack_pop ( void );
char *	dirstack_fullpath ( void );
char *	dirstack_filename ( void );
int	dirstack_empty ( void );

songdata * songdata_init ( Config * conf,  int init_colors[] );
void songdata_randomize(songdata *);
void songdata_shutdown ( void );

songdata_song * new_songdata_song(void);
void songdata_reload_search_results();


#endif
