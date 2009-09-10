#include "window_playback.h"

#include "gui.h"
#include "controller/keyboard_controller.h"

Window * playback;

Window * window_playback_init ( Config * conf )
{
	playback = malloc( sizeof ( Window ) );

	/*
	* in theory, this window should NEVER be active, but just in case ...
	*/
	playback->update = update_info;	// generic dummy update
	playback->activate = active_win;
	playback->deactivate = inactive_win;
	playback->prev = NULL;	// can't tab out of this!
	playback->next = NULL;
	playback->input = keyboard_controller_read_key;
	playback->flags |= W_RDONLY;
	playback->yoffset = 1;
    playback->flags = 0x0;
	playback->x = conf->playback_window.x;
	playback->y = conf->playback_window.y;
	playback->height = conf->playback_window.height;
	playback->width = conf->playback_window.width;
	playback->title_dfl = conf->playback_window.title_dfl;
	playback->title_fmt = conf->playback_window.title_fmt;
	playback->format = conf->playback_window.format;
	playback->name = window_playback;

	if ( playback->height < 6 )
		playback->height = 2;
	if ( playback->width == 0 )
		playback->width = COLS - playback->x;

	playback->win = newwin ( playback->height, playback->width, playback->y, playback->x );
	playback->panel = new_panel ( playback->win );
	keypad ( playback->win, TRUE );
	wbkgd ( playback->win, conf->colors[PLAYBACK_WINDOW] );
	return playback;
}
void window_playback_update ( void )
{
	playback->update ( playback );
}
void window_playback_activate ( void )
{
	playback->activate ( playback );
}
void window_playback_deactivate ( void )
{
	playback->deactivate ( playback );
}
void window_playback_shutdown ( void )
{
  delwin(playback->win);
  del_panel(playback->panel);
	free ( playback );
}
