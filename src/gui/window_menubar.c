#include "window_menubar.h"

#include "gui.h"
#include "controller/keyboard_controller.h"
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

Window * menubar;
Config * config;

int progress_length, progress_progress, progress_animate, progress_bar_running;

Window * window_menubar_init ( Config * conf )
{
    config = conf;
	menubar = malloc( sizeof ( Window ) );

	menubar->activate = window_menubar_standard;
	menubar->update = window_menubar_standard;
	menubar->deactivate = window_menubar_clear;
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
  window_menubar_clear ( window );
  my_mvwaddstr ( window->win, 0, ( ( x-strlen ( window->title_dfl ) ) /2 ), config->colors[MENU_TEXT], window->title_dfl );
  snprintf ( version_str, 128, "%.2d-%.2d-%.4d %.2d:%.2d v%s", currentTime->tm_mday, currentTime->tm_mon + 1, currentTime->tm_year + 1900, currentTime->tm_hour, currentTime->tm_min,  "4.0rc1" /* TODO: VERSION*/ );
  my_mvwaddstr ( window->win, 0, x - strlen ( version_str ) + 2, config->colors[MENU_TEXT], version_str );

  update_panels();
  if ( config->c_flags & C_FIX_BORDERS )
    redrawwin ( window->win );
  doupdate();
  return 1;
}

int window_menubar_clear ( Window *window )
{
  wmove ( window->win, 0, 0 );
  wclrtoeol ( window->win );
  wbkgd ( window->win, config->colors[MENU_WINDOW] );
  return 1;
}

void window_menubar_progress_bar_init(char * title){
  char buf[512];
  
  progress_length = menubar->width - (strlen(title) + strlen(" [] |") + 1);
  progress_progress = 0;
  progress_animate = 0;
  progress_bar_running = 1;
  
  window_menubar_deactivate();
  sprintf(buf, "%s [", title);
  my_mvwaddstr ( menubar->win, 0, 0, config->colors[MENU_TEXT], buf);
  my_mvwaddstr ( menubar->win, 0, menubar->width - 3, config->colors[MENU_TEXT], "] /");
  update_panels();
  doupdate();
}

void window_menubar_progress_bar_animate(){
  if(progress_bar_running){
    char img;
    char buf[512];
    if ( ( progress_animate & 0x03 ) == 0x00 ){
      img = '|';
    }else if ( ( progress_animate & 0x03 ) == 0x01 ){
      img = '/';
    }else if ( ( progress_animate & 0x03 ) == 0x02 ){
      img = '-';
    }else{
      img = '\\';
    }
    sprintf(buf, "%c", img);
    progress_animate++;
    my_mvwaddstr ( menubar->win, 0, menubar->width - 1, config->colors[MENU_TEXT], buf);
    update_panels();
    doupdate();
  }
}

void window_menubar_progress_bar_progress(int pcts){
  char line[menubar->width];
  memset(&line, 0, menubar->width);
  int total = (progress_length * pcts) / 100;
  int i = 0;
  for(; i <= total; i++){
    sprintf(line, "%s%c", line , '*');
  }
  my_mvwaddstr ( menubar->win, 0, (menubar->width - 4) - progress_length, config->colors[MENU_TEXT], line);
  update_panels();
  doupdate();
}

void window_menubar_progress_bar_remove(){
  progress_length = 0;
  progress_progress = 0;
  progress_animate = 0;
  progress_bar_running = 0;
  
  window_menubar_activate();
}

