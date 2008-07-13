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
  int xoffset;                       /* x offset for the data */
  int yoffset;                       /* y offset for the data */
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
