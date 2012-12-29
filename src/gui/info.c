#include "gui.h"

void gui_init_info(void) {
	info_window = window_new();
	info_window->update = gui_update_info;
	info_window->yoffset = 0;
	
	if (conf->info_window.height == 0) {
		conf->info_window.height = LINES - conf->info_window.y;
	}
	if (conf->info_window.width < 4) {
		conf->info_window.width = COLS - conf->info_window.x;
	}
	window_init(info_window, WINDOW_NAME_INFO, &conf->info_window);
}

void gui_update_info(void) {
	info_window->file = active_window->list->selected;
	window_draw_info(info_window);
	window_update(info_window);
}

void gui_shutdown_info(void) {
	window_free(info_window);
}
