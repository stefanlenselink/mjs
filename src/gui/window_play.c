#include "window_play.h"

#include "gui.h"
#include "controller/keyboard_controller.h"
#include "engine/engine.h"

Window * play;

Window * window_play_init ( Config * conf )
{
	play = malloc( sizeof ( Window ) );

	play->update = show_list;
	play->activate = active_win;
	play->deactivate = inactive_win;
    play->input = keyboard_controller_read_key;
    play->flags = 0x0;
	play->flags |= W_LIST;

	play->x = conf->play_window.x;
	play->y = conf->play_window.y;
	play->height = conf->play_window.height;
	play->width = conf->play_window.width;
	play->title_dfl = conf->play_window.title_dfl;
	play->title_fmt = conf->play_window.title_fmt;
	play->format = conf->play_window.format;
	play->name = window_play;

	if ( play->height == 0 )
		play->height = LINES - play->y;
	if ( play->width < 4 )
		play->width = COLS - play->x;

	play->win = newwin ( play->height, play->width, play->y, play->x );
	play->panel = new_panel ( play->win );
	keypad ( play->win, TRUE );
	wbkgd ( play->win, conf->colors[PLAY_WINDOW] );
	return play;
}
void window_play_update ( void )
{
	play->update ( play );
}
void window_play_activate ( void )
{
	play->activate ( play );
}
void window_play_deactivate ( void )
{
	play->deactivate ( play );
}
void window_play_shutdown ( void )
{
  delwin(play->win);
  del_panel(play->panel);
	free ( play );
}
void window_play_notify_title_changed(void){
  engine_load_current_meta_info(play->contents.list->playing);
  window_play_update();
}
