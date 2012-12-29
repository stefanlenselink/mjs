#ifndef _gui_h
#define _gui_h

#include "mjs.h"
#include "window.h"

void gui_init(void);
void gui_loop(void);
void gui_update_filelist(void);
void gui_update_playlist(void);
void gui_update_playback(void);
void gui_update_playback_time(void);
void gui_update_info(void);
void gui_update_bar(void);
int gui_ask_yes_no(char *question);
int gui_ask(char *question, char *answer);
void gui_progress_start(char *title);
void gui_progress_animate(void);
void gui_progress_value(int pcts);
void gui_progress_stop(void);
void gui_shutdown(void);

// private
Window *active_window;
Window *filelist_window;
Window *playlist_window;
Window *playback_window;
Window *info_window;
Window *bar_window;

void gui_init_filelist(void);
void gui_init_playlist(void);
void gui_init_playback(void);
void gui_init_info(void);
void gui_init_bar(void);
void gui_activate_filelist(void);
void gui_activate_playlist(void);
void gui_activate_bar(void);
void gui_deactivate_filelist(void);
void gui_deactivate_playlist(void);
void gui_deactivate_bar(void);
void gui_input_filelist(int c);
void gui_input_playlist(int c);
void gui_shutdown_filelist(void);
void gui_shutdown_playlist(void);
void gui_shutdown_playback(void);
void gui_shutdown_info(void);
void gui_shutdown_bar(void);

#endif /* _gui_h */
