#include "window_files.h"

#include "gui.h"
#include "controller/keyboard_controller.h"

#include <stdio.h>

Window * files;

Window * window_files_init ( Config * conf )
{
	files = malloc( sizeof ( Window ) );

	files->update = show_list;
	files->activate = active_win;
	files->deactivate = inactive_win;
	files->input = keyboard_controller_read_key;
    files->flags = 0x0;
	files->flags |= W_LIST | W_RDONLY;

	files->x = conf->files_window.x;
	files->y = conf->files_window.y;
	files->height = conf->files_window.height;
	files->width = conf->files_window.width;
	files->title_dfl = conf->files_window.title_dfl;
	files->title_fmt = conf->files_window.title_fmt;
	files->format = conf->files_window.format;
	files->name = window_files;

	if ( files->height == 0 )
		files->height = LINES - files->y;
	if ( files->width < 4 )
		files->width = COLS - files->x;
	files->win = newwin ( files->height, files->width, files->y, files->x );
	files->panel = new_panel ( files->win );
	keypad ( files->win, TRUE );
	wbkgd ( files->win, conf->colors[FILE_WINDOW] );
	return files;
}
void window_files_update ( void )
{
	files->update ( files );
}
void window_files_activate ( void )
{
	files->activate ( files );
}
void window_files_deactivate ( void )
{
	files->deactivate ( files );
}
void window_files_shutdown ( void )
{
  del_panel(files->panel);
  delwin(files->win);
  free ( files );
}
