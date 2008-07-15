#include "window_menubar.h"

#include "gui.h"
#include "controller/keyboard_controller.h"

Window * menubar;

Window * window_menubar_init ( Config * conf )
{
	menubar = calloc ( 1, sizeof ( Window ) );

	menubar->activate = std_menubar;
	menubar->update = std_menubar;
	menubar->deactivate = clear_menubar;
	menubar->input = keyboard_controller_read_key;

	menubar->x = conf->menubar_window.x;
	menubar->y = conf->menubar_window.y;
	menubar->height = conf->menubar_window.height;
	menubar->width = conf->menubar_window.width;
	menubar->title_dfl = conf->menubar_window.title_dfl;
	menubar->title_fmt = conf->menubar_window.title_fmt;
	menubar->format = conf->menubar_window.format;
	menubar->name = window_menubar;

	if ( menubar->height == 0 )
		menubar->height = 1;
	if ( menubar->width == 0 )
		menubar->width = COLS - menubar->x;

	menubar->win = newwin ( menubar->height, menubar->width, menubar->y, menubar->x );
	menubar->panel = new_panel ( menubar->win );
	keypad ( menubar->win, TRUE );
	wbkgd ( menubar->win, conf->colors[MENU_WINDOW] );
	return menubar;
}
void window_menubar_update ( void )
{
	menubar->update ( menubar );
}
void window_menubar_activate ( void )
{
	menubar->activate ( menubar );
}
void window_menubar_deactivate ( void )
{
	menubar->deactivate ( menubar );
}
void window_menubar_shutdown ( void )
{
	free ( menubar );
}
