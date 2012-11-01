#include "defs.h"
#include "gui.h"
#include "tokens.h"
#include "songdata/songdata.h"
#include "controller/controller.h"

#include "window_playback.h"
#include "window_play.h"
#include "window_menubar.h"
#include "window_files.h"
#include "window_info.h"
#include "engine/engine.h"
#include "mjs.h"


#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>


static char	*parse_title ( Window *, char *, int );
static void init_info ( Window * );
void gui_init_color_pairs();

Config * conf;

Window * playback;
Window * menubar;
Window * files;
Window * info;
Window * play;
Window * active;

/* colors */
int * colors;
int last_elapsed = 1; //Foull the guys

int
show_list ( Window *window )
{
	int x = window->width-4, y = window->height-2, i;
	char buf[BUFFER_SIZE+1];
	const char *line;
	int color;
	WINDOW *win = window->win;
	songdata_song *ftmp;
	songdata *list = window->contents.list;

	if ( !list )
		return 0;

	// do we need to reposition the screen
	if ( list->length > y-1 )
	{
		if ( ( list->where - 4 ) == list->wheretop )
		{
			list->wheretop--;
			if ( list->wheretop < 0 )
				list->wheretop = 0;
		}
		else if ( ( list->where + 3 ) == ( list->wheretop + y ) )
		{
			if ( ++list->wheretop > ( list->length - y ) )
				list->wheretop = list->length - y;
		}
		else if ( ( ( list->where - 5 ) < list->wheretop ) || ( ( list->where + 3 ) > ( list->wheretop + y ) ) )
		{
			list->wheretop = list->where - ( y/2 );
			if ( list->wheretop <  0 )
				list->wheretop = 0;
			if ( list->wheretop > ( list->length - y ) )
				list->wheretop = list->length - y;
		}
	}
	// find the list->top entry
	list->top=list->head;
	if ( ( list->wheretop > 0 ) & ( list->length > y ) )
	{
		for ( i = 0; ( i < list->wheretop ) && ( list->top->next ); i++ )
			list->top = list->top->next;
	}

	werase(win);

	ftmp = list->top;
	for ( i = 0; i < y; i++ )
	{
		if ( ftmp )
		{
			if ( window->format )
			{
				memset ( buf, 0, sizeof ( buf ) );
				line = parse_tokens ( window,ftmp, buf, BUFFER_SIZE, window->format );
			}
			else
				line = ftmp->filename;
			if ( window == play )
			{
				if ( ftmp == list->playing )
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[PLAY_SELECTED_PLAYING];
					else
						color = colors[PLAY_UNSELECTED_PLAYING];
				else
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[PLAY_SELECTED];
					else
						color = colors[PLAY_UNSELECTED];
			}
			else	// window==files
				if ( ftmp->flags & ( F_DIR | F_PLAYLIST ) )
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[FILE_SELECTED_DIRECTORY];
					else
						color = colors[FILE_UNSELECTED_DIRECTORY];
				else
					if ( ( ftmp == list->selected ) && ( window->flags & W_ACTIVE ) )
						color = colors[FILE_SELECTED];
					else
						color = colors[FILE_UNSELECTED];
			if ( ftmp->flags & F_PAUSED )
				color |=  A_BLINK;

			wattrset(win, color);
			mvwaddnstr(win, i + 1, 2, line, x);

			ftmp = ftmp->next;
		}
	}

	update_border(window);
	do_scrollbar(window);
	update_title(window);
	update_panels();
	doupdate();

	return 1;
}

Window * move_files_selector ( int c )
{
	return move_selector ( files, c );
}

Window *
move_selector ( Window *window, int c )
{
	songdata_song *file;
	songdata *list = window->contents.list;
	int j, maxx, maxy, length;

	if ( !list )
		return NULL;
	if ( !list->selected )
		return NULL;

	//BUG gevonden in owee: list->startposSelected = 0; //Reset the scrolling pos in the songdata_song

	getmaxyx ( window->win, maxy, maxx );
	length = maxy - 1;

	switch ( c )
	{
		case KEY_ENTER:
		case '\n':
		case '\r':
			if ( ( window == play ) && ( list->playing ) )
			{
				list->selected = songdata_next_valid ( list, list->head, KEY_HOME );
				list->where = 1;
				list->wheretop = 0;

				for ( j = 0; list->selected->next && list->selected != list->playing; j++ )
				{
					list->selected = songdata_next_valid ( list, list->selected->next, KEY_DOWN );
					list->where++;
				}
				return window;
			}
			break;
		case KEY_HOME:
			list->selected = songdata_next_valid ( list, list->head, c );
			list->where = 1;
			list->wheretop = 0;
			return window;
		case KEY_END:
			list->selected = songdata_next_valid ( list, list->tail, c );
			list->where = list->length;
			return window;
		case KEY_DOWN:
			if ( ( file = songdata_next_valid ( list, list->selected->next, c ) ) )
			{
				list->selected = file;
				list->where++;
				return window;
			}
			break;
		case KEY_UP:
			if ( ( file = songdata_next_valid ( list, list->selected->prev, c ) ) )
			{
				list->selected = file;
				list->where--;
				return window;
			}
			break;
		case KEY_NPAGE:
			for ( j = 0; list->selected->next && j < length-1; j++ )
			{
				list->selected = songdata_next_valid ( list, list->selected->next, KEY_DOWN );
				list->where++;
			}
			list->selected = songdata_next_valid ( list, list->selected, c );
			return window;
		case KEY_PPAGE:
			for ( j = 0; list->selected->prev && j < length-1; j++ )
			{
				list->selected = songdata_next_valid ( list, list->selected->prev, KEY_UP );
				list->where--;
			}
			list->selected = songdata_next_valid ( list, list->selected, c );
			return window;
		default:
			break;
	}
	return NULL;
}

int
update_info ( Window *window )
{
	WINDOW *win = window->win;
	int i = window->width;
	songdata_song *file = NULL;
    playback->contents.show = &play->contents.list->playing;
	file = *window->contents.show;

	char *title = "";
	char *artist = "";
	char *album = "";
	char *genre = "";

	werase(win);

	if (file) {
		if (file->flags & F_DIR) {
			title = "(Directory)";
		} else {
			title = file->title;
			artist = file->artist? file->artist: "Unknown";
			album = file->album? file->album: "Unknown";
			genre = file->genre? file->genre: "Unknown";
		}
	}

	wattrset(win, colors[INFO_TEXT]);
	mvwprintw(win, 1+window->yoffset, 2, "Title : %s", title);
	mvwprintw(win, 2+window->yoffset, 2, "Artist: %s", artist);
	mvwprintw(win, 3+window->yoffset, 2, "Album : %s", album);
	if (conf->c_flags & C_USE_GENRE) {
		mvwprintw(win, 4+window->yoffset, 2, "Genre : %s", genre);
	}

	update_border(window);
	update_title ( window );
    gui_update_play_time();
    doupdate();

	return 1;
}

void change_active ( int next )
{
	Window * new;
	if ( next )
	{
		if ( active->next == NULL )
		{
			return;
		}
		new = active->next;
	}
	else
	{
		if ( active->prev == NULL )
		{
			return;
		}
		new = active->prev;
	}
	active->deactivate ( active );
	active->update ( active );
	active = new;
	active->activate ( active );
	active->update ( active );
	info->contents.show = &active->contents.list->selected;
	info->update ( info );
	doupdate();
}

int update_border(Window *window) {
	if (window->flags & W_ACTIVE)
		wattrset(window->win, colors[WIN_ACTIVE]);
	else
		wattrset(window->win, colors[WIN_INACTIVE]);
	box(window->win, 0, 0);

	return 1;
}

int
active_win ( Window *window )
{
	WINDOW *win = window->win;
	PANEL *panel = window->panel;
	window->flags |= W_ACTIVE;
	update_border(window);
	update_title ( window );
	top_panel ( panel );
	update_panels();

	return 1;
}

int
inactive_win ( Window *window )
{
	WINDOW *win = window->win;
	window->flags &= ~W_ACTIVE;
	update_border(window);
	update_title ( window );
	update_panels();

	return 1;
}

int
update_title ( Window *window )
{
	WINDOW *win = window->win;
	char title[BUFFER_SIZE+1], *p = NULL;
	int color;
	int i = 0, offset, x = window->width-2;

	if ( window->flags & W_ACTIVE )
		color = colors[WIN_ACTIVE_TITLE];
	else
		color = colors[WIN_INACTIVE_TITLE];

	memset(title, 0, sizeof(title));
	p = parse_title(window, title, BUFFER_SIZE);
	i = strlen(p);

	if (i == 0) {
		p = (char *)window->title_dfl;
		i = strlen(p);
	}

	if (i+4 > x) {
		p[x-4] = '\0';
		i = x-4;
	}

	i += 4;

	offset = (x-i) / 2;

	wattrset(win, color);
	mvwprintw(win, 0, 1+offset, "[ %s ]", p);

	return 0;
}

void
do_scrollbar ( Window *window )
{
	int x, y; /* window dimensions, etc */
	int top, bar; /* scrollbar portions */
	double value; /* how much each notch represents */
	int color, barcolor;
	songdata *list = window->contents.list;
	WINDOW *win = window->win;

	getmaxyx ( win, y, x );
	y -= 2;
	x -= 1;

	if (list->length < y + 1)
		return;

	value = list->length / ( double ) y;

	/* calculate the sizes of our status bar */
	top = (int)(list->wheretop / value + 0.5);
	bar = (int)(y / value + .5);

	if (bar < 1)
		bar = 1;

	/* because of rounding we may end up with too much, correct for that */
	if (top + bar > y)
		bar = y - top;

	if (window->flags & W_ACTIVE) {
		color = colors[WIN_ACTIVE_SCROLL];
		barcolor = colors[WIN_ACTIVE_SCROLLBAR];
	} else {
		color = colors[WIN_INACTIVE_SCROLL];
		barcolor = colors[WIN_INACTIVE_SCROLLBAR];
	}

	wattrset(win, color);
	mvwvline (win, 1, x, ACS_CKBOARD, y);
	wattrset(win, barcolor);
	mvwvline (win, 1 + top, x, ACS_CKBOARD, bar);

	update_panels();
}

char *
parse_title ( Window *win, char *title, int len )
{
	char *p = ( char * ) win->title_dfl;

	if ( win->title_fmt )
	{
		if ( win->flags & W_LIST && win->contents.list && win->contents.list->selected )
			p = ( char * ) parse_tokens ( win, win->contents.list->selected, title, len, win->title_fmt );
		else if ( ! ( win->flags & W_LIST ) && ( win==info ) )
			p = ( char * ) parse_tokens ( win, *win->contents.show, title, len, win->title_fmt );
//		else if (info->contents.show)
//			p = (char *)parse_tokens(win, *info->contents.show, title, len, win->title_fmt);
	}

	return p;
}


void gui_init ( Config * init_conf,   int init_colors[], songdata * mp3list, songdata * playlist )
{
	conf = init_conf;
	colors = init_colors;

	curs_set ( 0 );
	leaveok ( stdscr, TRUE );
	cbreak ();
	noecho ();
	use_default_colors ();
	start_color ();
	nonl ();
	gui_init_color_pairs();
	wbkgd ( stdscr, colors[FILE_WINDOW] );

	/*
	* malloc() for the windows and set up our initial callbacks ...
	*/
	info = window_info_init ( conf );
	nodelay(info->win, FALSE);
	play = window_play_init ( conf );
	nodelay(play->win, FALSE);
	playback = window_playback_init ( conf );
	nodelay(playback->win, FALSE);
	menubar = window_menubar_init ( conf );
	nodelay(menubar->win, FALSE);
	files = window_files_init ( conf );
	nodelay(files->win, FALSE);
	active = files;

	files->prev = play;
	files->next = play;
	info->prev = files;
	info->next = play;
	play->prev = files;
	play->next = files;

	/* Eigenlijk nog verplaatsen */
	files->contents.list = mp3list;
	info->contents.show = &mp3list->selected;
	play->contents.list = playlist;

	if ( !files->win || !info->win || !play->win || !menubar->win )
		bailout ( 1 );

	menubar->activate ( menubar );
	init_info ( info );
	init_info ( playback );
	play->deactivate ( play );

	info->deactivate ( info );
	active->activate ( active );
	playback->deactivate ( playback );

	play->update ( play );
	files->update ( files );
	info->update ( info );
	gui_update_play_time();

	doupdate ();
}

void gui_init_color_pairs() {
	int fore, back;
	for (fore = 0; fore < COLORS; fore++) {
		for (back = 0; back < COLORS; back++) {
			init_pair(fore << 3 | back, fore, back);
		}
	}
}

static void
init_info ( Window * window )
{
	WINDOW * win = window->win;

	update_info(window);
	update_panels ();
	doupdate ();
}

void gui_update_play_time ( void ) {
	int elapsed, remaining, length;
    elapsed = engine_get_elapsed();
    //If engine == paused keep displaying....Impact on performance...
    if(last_elapsed != elapsed || engine_is_paused()){
      int color = engine_is_paused() ? colors[PLAYBACK_TEXT] | A_BLINK : colors[PLAYBACK_TEXT];
      last_elapsed = elapsed;
      remaining = engine_get_remaining();
      length = engine_get_length();

      wattrset(playback->win, color);
      if(!length){
        mvwprintw(playback->win, 1, 1, " Time  : %02d:%02d:%02d                      ",
          (int)elapsed / 3600, //Aantal Uur
          (int)elapsed / 60, //Aantal Minuten
          ((int)elapsed) % 60 //Aantal Seconden
        );
      }else if(length > 3600){
        mvwprintw(playback->win, 1, 1, " Time  : %02d:%02d:%02d / %02d:%02d:%02d (%02d:%02d:%02d)",
          (int)elapsed / 3600, //Uren
          (int)(elapsed % 3600) / 60, //Minuten
          (int)(elapsed % 3600) % 60, //Seconden
          (int)remaining / 3600, //Uren
          (int)(remaining % 3600) / 60, //Minuten
          (int)(remaining % 3600) % 60, //Seconden
          (int)length / 3600, //Uren
          (int)(length % 3600) / 60, //Minuten
          (int)(length % 3600) % 60  //Seconden
        );
      }else{
        mvwprintw(playback->win, 1, 1, " Time  : %02d:%02d / %02d:%02d (%02d:%02d)         ",
          (int)elapsed / 60, //Minuten
          (int)elapsed % 60, //Seconden
          (int)remaining / 60, //Minuten
          (int)remaining % 60, //Seconden
          (int)length / 60, //Minuten
          (int)length % 60  //Seconden
        );
      }
      update_panels();
      doupdate ();
    }
}

void gui_shutdown ( void )
{
  /*General NCurses shutdown*/
  wclear ( stdscr );
  refresh ();
  endwin ();
//  delscreen(stdscr);
  
  window_files_shutdown();
  window_info_shutdown();
  window_menubar_shutdown();
  window_playback_shutdown();
  window_play_shutdown();
    
}

void poll_keyboard ( void )
{
	//Stupid startup ordering bug
	if(active == NULL){
		sleep(1); //Not putting a sleep causes a 'poll run'
		return;
	}
	active->input ( active );
}

static void print_question(char * question)
{
  window_menubar_deactivate();
  wattrset(menubar->win, colors[MENU_TEXT] | A_BLINK);
  mvwaddstr(menubar->win, 0, 0, question);
  update_panels();
  doupdate();
}

int gui_ask_yes_no_question(char * question)
{
  unsigned short c;
  print_question(question);
  c = wgetch ( menubar->win ); //TODO keyboard handeling -> Move to keyboard_controller some how
  window_menubar_activate();
  return ( c == 'y' ) || ( c == 'Y' );
}

int gui_ask_question(char * question, char * answer){
  int c, i = 0;
  print_question(question);
  echo();
  wgetnstr(menubar->win, answer, 512);
  noecho();
  window_menubar_activate();
  
  return strlen(answer);
}

