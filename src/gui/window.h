#ifndef _window_h
#define _window_h

#include "songdata/songdata.h"

#include <ncurses.h>
#include <panel.h>

typedef enum
{
	window_files,
	window_info,
	window_menubar,
	window_playback,
	window_play
}
WindowName;

typedef struct _win
{
	WINDOW *win;                       /* the actual ncurses window            */
	PANEL *panel;                      /* the panel, duh                       */
	int width;                         /* window/panel width                   */
	int height;                        /* window/panel height                  */
	int x;                             /* x-coord for the upper-left corner    */
	int y;                             /* y-coord for the upper-left corner    */
	int xoffset;                       /* x offset for the data */
	int yoffset;                       /* y offset for the data */
	int ( *input ) ( struct _win * );  /* Function to parse input              */
	int ( *update ) ( struct _win * ); /* Updates the window                   */
	int ( *activate ) ( struct _win * );   /* activate this window                 */
	int ( *deactivate ) ( struct _win * ); /* deactivate the window                */
	short int flags;                   /* various info about the window        */
#define W_ACTIVE   0x01              /* is the window active?                */
#define W_LIST     0x02              /* is it a list, or just 'contents'     */
#define W_RDONLY   0x04              /* is it read-only, ie can we modify it */
	union
	{
		wlist *list;               /* if the window has contents, use this */
		flist **show;               /* otherwise just this                  */
	} contents;
	const u_char *title_fmt;           /* format string for window title       */
	const u_char *title_dfl;           /* default (if needed) window title     */
	u_char *title;                     /* the window title                     */
	const u_char *format;              /* format for the text in the window    */
	struct _win *next;                 /* next window in the cycle             */
	struct _win *prev;                 /* previous window in the cycle         */
	WindowName name;
} Window;

#endif /* _window_h */
