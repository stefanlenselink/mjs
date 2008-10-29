#include "window_info.h"

#include "gui.h"
#include "controller/keyboard_controller.h"
Window * info;

Window * window_info_init ( Config * conf )
{ 
	info = malloc( sizeof ( Window ) );

	info->update = update_info;
	info->activate = active_win;
	info->deactivate = inactive_win;
    info->prev = NULL;
    info->next = NULL;
	info->input = keyboard_controller_read_key;
	info->flags |= W_RDONLY;
    info->yoffset = 0;
    
	info->x = conf->info_window.x;
	info->y = conf->info_window.y;
	info->height = conf->info_window.height;
	info->width = conf->info_window.width;
	info->title_dfl = conf->info_window.title_dfl;
	info->title_fmt = conf->info_window.title_fmt;
	info->format = conf->info_window.format;
	info->name = window_info;
    

	if ( info->width < 4 )
		info->width = COLS - info->x;

	info->win = newwin ( info->height, info->width, info->y, info->x );
	info->panel = new_panel ( info->win );
	keypad ( info->win, TRUE );
	wbkgd ( info->win, conf->colors[INFO_WINDOW] );
	return info;
}
void window_info_update ( void )
{
	info->update ( info );
}
void window_info_activate ( void )
{
	info->activate ( info );
}
void window_info_deactivate ( void )
{
	info->deactivate ( info );
}
void window_info_shutdown ( void )
{
	free ( info );
}
