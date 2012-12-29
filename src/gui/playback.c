#include "gui.h"

static int last_elapsed = 1; //Foull the guys

void gui_init_playback(void) {
	playback_window = window_new();
	playback_window->update = gui_update_playback;
	playback_window->yoffset = 1;
	
	if (conf->playback_window.height == 0) {
		conf->playback_window.height = LINES - conf->playback_window.y;
	}
	if (conf->playback_window.width < 4) {
		conf->playback_window.width = COLS - conf->playback_window.x;
	}
	window_init(playback_window, WINDOW_NAME_PLAYBACK, &conf->playback_window);
}

void gui_update_playback(void) {
	playback_window->file = playlist->playing;
	window_draw_info(playback_window);
	gui_update_playback_time();
}

void gui_update_playback_time(void) {
	int elapsed, remaining, length, color;
	
	elapsed = engine_get_elapsed();
	if (last_elapsed != elapsed) {
		color = engine_is_paused()? conf->colors[PLAYBACK_TEXT] | A_BLINK : conf->colors[PLAYBACK_TEXT];
		last_elapsed = elapsed;
		remaining = engine_get_remaining();
		length = engine_get_length();

		wattrset(playback_window->win, color);
		if (!length) {
			mvwprintw(playback_window->win, 1, 1, " Time  : %02d:%02d:%02d                      ",
					(int) elapsed / 3600, //Aantal Uur
					(int) elapsed / 60, //Aantal Minuten
					((int) elapsed) % 60 //Aantal Seconden
			);
		} else if (length > 3600) {
			mvwprintw(playback_window->win, 1, 1, " Time  : %02d:%02d:%02d / %02d:%02d:%02d (%02d:%02d:%02d)",
					(int) elapsed / 3600, //Uren
					(int) (elapsed % 3600) / 60, //Minuten
					(int) (elapsed % 3600) % 60, //Seconden
					(int) remaining / 3600, //Uren
					(int) (remaining % 3600) / 60, //Minuten
					(int) (remaining % 3600) % 60, //Seconden
					(int) length / 3600, //Uren
					(int) (length % 3600) / 60, //Minuten
					(int) (length % 3600) % 60 //Seconden
			);
		} else {
			mvwprintw(playback_window->win, 1, 1, " Time  : %02d:%02d / %02d:%02d (%02d:%02d)         ",
					(int) elapsed / 60, //Minuten
					(int) elapsed % 60, //Seconden
					(int) remaining / 60, //Minuten
					(int) remaining % 60, //Seconden
					(int) length / 60, //Minuten
					(int) length % 60 //Seconden
			);
		}
	}
	window_update(playback_window);
}

void gui_shutdown_playback(void) {
	window_free(playback_window);
}
