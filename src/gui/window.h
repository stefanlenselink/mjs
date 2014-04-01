#ifndef _window_h
#define _window_h

#include "songdata/songdata.h"
#include "config/config.h"

#include <ncurses.h>
#include <panel.h>

typedef enum {
	WINDOW_NAME_FILELIST,
	WINDOW_NAME_PLAYLIST,
	WINDOW_NAME_PLAYBACK,
	WINDOW_NAME_INFO,
	WINDOW_NAME_BAR
} WindowName;

typedef enum {
	WINDOW_TYPE_GENERIC,
	WINDOW_TYPE_LIST,
	WINDOW_TYPE_INFO
} WindowType;

typedef struct _Window Window;



struct _Window {
	WINDOW *win; // the actual ncurses window
	WINDOW *win_cont; // sub window for the content
	PANEL *panel; // the panel, duh
	int active;
	int width; // window/panel width
	int height; // window/panel height
	int x; // x-coord for the upper-left corner
	int y; // y-coord for the upper-left corner
	int xoffset; // x offset for the data
	int yoffset; // y offset for the data
	char *title_fmt; // format string for window title
	char *title_dfl; // default (if needed) window title
	char *title; // the window title
	void (*input)(int c); // Function to parse input
	void (*update)(void); // Updates the window
	void (*activate)(void); // activate this window
	void (*deactivate)(void); // deactivate the window
	Window *next; // next window in the cycle
	Window *prev; // previous window in the cycle
	WindowName name;
	WindowType type;
	int flags;

	struct /*_WindowList*/ {
		songdata *list;
		char *format; // format for the text in the window
	};

	struct /*_WindowInfo*/ {
		songdata_song *file; // otherwise just this	
	};
};

Window *window_new(void);
void window_init(Window *window, WindowName name, WindowConfig *config);
void window_activate(Window *window);
void window_deactivate(Window *window);
void window_input(Window *window, int c);
void window_input_list(Window *window, int c);
void window_draw_title(Window *window);
void window_draw_border(Window *window);
void window_draw_scrollbar(Window *window);
void window_draw_info(Window *window);
void window_draw_list(Window *window);
void window_update(Window *window);
void window_free(Window *window);

#endif /* _window_h */
