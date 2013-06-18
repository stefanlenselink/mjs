#include "gui.h"

void gui_init_playlist(void) {
	playlist_window = window_new();
	playlist_window->list = playlist;
	playlist_window->type = WINDOW_TYPE_LIST;
	playlist_window->update = gui_update_playlist;
	playlist_window->activate = gui_activate_playlist;
	playlist_window->deactivate = gui_deactivate_playlist;
	playlist_window->input = gui_input_playlist;

	if (conf->play_window.height == 0) {
		conf->play_window.height = LINES - conf->play_window.y;
	}
	if (conf->play_window.width < 4) {
		conf->play_window.width = COLS - conf->play_window.x;
	}	
	window_init(playlist_window, WINDOW_NAME_PLAYLIST, &conf->play_window);
}

void gui_update_playlist(void) {
	if(!playlist_window){
		return;
	}
	window_draw_border(playlist_window);
	window_draw_title(playlist_window);
	window_draw_scrollbar(playlist_window);
	window_draw_list(playlist_window);
	window_update(playlist_window);

	//Update the displayed info in the info window.
	gui_update_info();
	gui_update();
}

void gui_activate_playlist(void) {
	window_activate(playlist_window);
}

void gui_deactivate_playlist(void) {
	window_deactivate(playlist_window);
}

void gui_input_playlist(int c) {
	int j;
	
	switch (c) {
		case '-':
			// Move selected forward in playlist
			if (playlist->selected != playlist->head) {
				controller_playlist_move_up();
				gui_update_playlist();
			}
			break;

		case '+':
		case '=':
			// Move selected backwards in playlist
			if (playlist->selected != playlist->tail) {
				controller_playlist_move_down();
				gui_update_playlist();
			}
			break;
		case KEY_ENTER:
		case '\n':
		case '\r':
			if (playlist->playing) {
				playlist->selected = songdata_next_valid(playlist, playlist->head, KEY_HOME);
				playlist->where = 1;
				playlist->wheretop = 0;

				for (j = 0; playlist->selected->next && playlist->selected != playlist->playing; j++) {
					playlist->selected = songdata_next_valid(playlist, playlist->selected->next, KEY_DOWN);
					playlist->where++;
				}
				gui_update_playlist();
			}
			break;
		case KEY_DC:
			// remove selected from playlist
			if (playlist->selected) {
				if (playlist->playing == playlist->selected && playlist->playing->next != NULL) {
					controller_next(playlist);
				} else if (playlist->playing == playlist->selected && playlist->playing->next == NULL) {
					controller_stop();
				}
				songdata_del(playlist, playlist->selected);
				gui_update_playlist();
				gui_update_info();
			}
			break;
		default:
			window_input(playlist_window, c);
			break;
	}
}

void gui_shutdown_playlist(void) {
	window_free(playlist_window);
}
