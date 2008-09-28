#include "window_menubar.h"

#include "gui.h"
#include "controller/keyboard_controller.h"
#include <time.h>
#include <string.h>

Window * menubar;
Config * config;
Window * window_menubar_init ( Config * conf )
{
    config = conf;
	menubar = malloc( sizeof ( Window ) );

	menubar->activate = window_menubar_standard;
	menubar->update = window_menubar_standard;
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

int window_menubar_standard ( Window *window )
{
  char version_str[128];
  struct tm * currentTime;
  time_t now2;
  int x = window->width-2;
  now2 = time ( NULL );
  currentTime = localtime ( &now2 );
  clear_menubar ( window );
  my_mvwaddstr ( window->win, 0, ( ( x-strlen ( window->title_dfl ) ) /2 ), config->colors[MENU_TEXT], window->title_dfl );
  snprintf ( version_str, 128, "%.2d-%.2d-%.4d %.2d:%.2d v%s", currentTime->tm_mday, currentTime->tm_mon + 1, currentTime->tm_year + 1900, currentTime->tm_hour, currentTime->tm_min,  "4.0rc1" /* TODO: VERSION*/ );
  my_mvwaddstr ( window->win, 0, x - strlen ( version_str ) + 2, config->colors[MENU_TEXT], version_str );

  update_panels();
  if ( config->c_flags & C_FIX_BORDERS )
    redrawwin ( window->win );
  doupdate();
  return 1;
}
