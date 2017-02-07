#include "gui.h"

#include "songdata/songdata.h"
#include "controller/controller.h"
#include "config/config.h"
#include "engine/engine.h" //TODO: Get rid of this.

#include <curses.h>
#include <term.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>


extern songdata * playlist;
extern songdata * mp3list;
extern Config * conf;

void gui_init_curs(void) {
	curs_set(0);
	leaveok(stdscr, TRUE);
	//cbreak();
	halfdelay(10);
	noecho();
	keypad(stdscr, TRUE);
	use_default_colors();
	start_color();
	nonl();
	wbkgd(stdscr, conf->colors[FILE_WINDOW]);
}

void gui_init_curs_color_pairs(void) {
	int fore, back;

	for (fore = 0; fore < COLORS; fore++) {
		for (back = 0; back < COLORS; back++) {
			init_pair(fore << 3 | back, fore, back);
		}
	}
}

void gui_handle_continue(int signum) {
	wrefresh(curscr);
	curs_set(0);
}

void gui_handle_alarm(int signum) {
	gui_update_bar();
}

void gui_init_signals(void) {
	struct itimerval rttimer;

	signal(SIGCONT, &gui_handle_continue);
	signal(SIGALRM, &gui_handle_alarm);

	rttimer.it_value.tv_sec = 60;
	rttimer.it_value.tv_usec = 0;
	rttimer.it_interval.tv_sec = 60;
	rttimer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &rttimer, NULL);
}

void gui_init(void) {
	gui_init_curs();
	gui_init_curs_color_pairs();
	gui_init_filelist();
	gui_init_playlist();
	gui_init_playback();
	gui_init_info();
	gui_init_bar();

	filelist_window->next = filelist_window->prev = playlist_window;
	playlist_window->next = playlist_window->prev = filelist_window;
	active_window = filelist_window;
	active_window->activate();

	gui_update_filelist();
	gui_update_playlist();
	gui_update_info();
	gui_update_playback();
	gui_update_bar();

	gui_init_signals();
}

void gui_update(void) {
	doupdate();
}

void gui_read(void) {
	int c = getch();
	char buf[512];

	switch (c) {
		case ERR:
			break;
		case KEY_REFRESH:
			gui_update();
			break;
		case KEY_F(1):
			// Exit mjs
			controller_exit();
			break;
		case KEY_F(2):
			// Clear playlist
			controller_clear_playlist();
			break;
		case KEY_F(3):
			if (gui_ask("Search for: ", buf)) {
				controller_search(buf);
			}
			break;
		case KEY_F(4):
			// Show last search results
			controller_reload_search_results();
			break;
		case KEY_F(5):
			// Randomize the playlist
			controller_shuffle_playlist();
			break;
		case KEY_F(6):
			// Save Playlist
			if ((conf->c_flags & C_ALLOW_P_SAVE) && playlist->head &&
					gui_ask("Save as: ", buf)) {
				controller_save_playlist(buf);
			}
			break;
		case KEY_F(7):
			// Stop the player
			controller_stop();
			break;
		case KEY_F(8):
			// Play / Pause the player
			controller_play_pause();
			break;
		case KEY_F(9):
			// Skip to previous mp3 in playlist
			controller_prev();
			break;
		case KEY_F(10):
			// FRWD
			engine_frwd(conf->jump, conf->jumpExpFactor);
			break;
		case KEY_F(11):
			//FFWD
			engine_ffwd(conf->jump, conf->jumpExpFactor);
			break;
		case KEY_F(12):
			// Skip to next mp3 in playlist
			controller_next();
			break;
		case '\t':
			active_window->deactivate();
			active_window = active_window->next;
			active_window->activate();
			break;
		case KEY_BTAB:
			active_window->deactivate();
			active_window = active_window->prev;
			active_window->activate();
			break;
		default:
			active_window->input(c);
			break;
	}
}

void gui_loop(void) {
	while (1) {
		gui_read();
		gui_update_playback_time();
		gui_update();
	}
}

void gui_shutdown(void) {
	gui_shutdown_bar();
	gui_shutdown_info();
	gui_shutdown_playback();
	gui_shutdown_playlist();
	gui_shutdown_filelist();

	wclear(stdscr);
	refresh();
	endwin();
	del_curterm(cur_term);
}
