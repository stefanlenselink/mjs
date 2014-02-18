#include "gui.h"

#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

static int progress_length;
static int progress_progress;
static int progress_animate;
static int progress_bar_running;

void gui_init_bar(void) {
	bar_window = malloc(sizeof(Window));
	bar_window->update = gui_update_bar;

	if (conf->menubar_window.height == 0) {
		conf->menubar_window.height = 1;
	}
	if (conf->menubar_window.width == 0) {
		conf->menubar_window.width = COLS - conf->menubar_window.x;
	}
	window_init(bar_window, WINDOW_NAME_BAR, &conf->menubar_window);
}

void gui_clear_bar(void) {
	wmove(bar_window->win, 0, 0);
	wclrtoeol(bar_window->win);
	wbkgd(bar_window->win, conf->colors[MENU_WINDOW]);
}

void gui_draw_bar(void) {
	char version_str[128];
	struct tm *currentTime;
	
	time_t now2;
	int x = bar_window->width - 2;
	now2 = time(NULL);
	currentTime = localtime(&now2);
	gui_clear_bar();
	wattrset(bar_window->win, conf->colors[MENU_TEXT]);
	mvwaddstr(bar_window->win, 0, ((x - strlen(bar_window->title_dfl)) / 2), bar_window->title_dfl);
	snprintf(version_str, 128, "%.2d-%.2d-%.4d %.2d:%.2d %s",
			currentTime->tm_mday,
			currentTime->tm_mon + 1,
			currentTime->tm_year + 1900,
			currentTime->tm_hour,
			currentTime->tm_min,
			VERSION);
	mvwaddstr(bar_window->win, 0, x - strlen(version_str) + 2, version_str);

	update_panels();
	if (conf->c_flags & C_FIX_BORDERS)
		redrawwin(bar_window->win);
	doupdate();
}

void gui_draw_question(char *question) {
	gui_clear_bar();
	wattrset(bar_window->win, conf->colors[MENU_TEXT] | A_BLINK);
	mvwaddstr(bar_window->win, 0, 0, question);
	update_panels();
	doupdate();
}

void gui_update_bar(void) {
	gui_draw_bar();
	window_update(bar_window);
}

int gui_ask_yes_no(char *question) {
	int c;
	
	gui_draw_question(question);
	halfdelay(50);
	c = wgetch(bar_window->win);
	halfdelay(10);
	gui_draw_bar();
	
	return c == 'y' || c == 'Y';
}

int gui_ask(char *question, char *answer) {
	gui_draw_question(question);
	echo();
	wgetnstr(bar_window->win, answer, 512);
	noecho();
	gui_draw_bar();

	return strlen(answer);
}

void gui_progress_start(char *title) {
	char buf[512];

	progress_length = bar_window->width - (strlen(title) + strlen(" [] |") + 1);
	progress_progress = 0;
	progress_animate = 0;
	progress_bar_running = 1;

	sprintf(buf, "%s [", title);
	wattrset(bar_window->win, conf->colors[MENU_TEXT]);
	mvwaddstr(bar_window->win, 0, 0, buf);
	mvwaddstr(bar_window->win, 0, bar_window->width - 3, "] /");
	update_panels();
	doupdate();
}

void gui_progress_animate(void) {
	int img;
	
	if (progress_bar_running) {
		switch (progress_animate & 0x03) {
		case 0:
			img = '|';
			break;
		case 1:
			img = '/';
			break;
		case 2:
			img = '-';
			break;
		case 3:
			img = '\\';
			break;
		}
		progress_animate++;
		
		wattrset(bar_window->win, conf->colors[MENU_TEXT]);
		mvwaddch(bar_window->win, 0, bar_window->width - 1, img);
		
		update_panels();
		doupdate();
	}
}

void gui_progress_value(int pcts) {
	char line[bar_window->width];
	int total = (progress_length * pcts) / 100;
	int i;
	
	memset(&line, 0, bar_window->width);

	for (i = 0; i <= total; i++) {
		sprintf(line, "%s%c", line, '*');
	}
	wattrset(bar_window->win, conf->colors[MENU_TEXT]);
	mvwaddstr(bar_window->win, 0, bar_window->width - 4 - progress_length, line);
	update_panels();
	doupdate();
}

void gui_progress_stop(void) {
	progress_length = 0;
	progress_progress = 0;
	progress_animate = 0;
	progress_bar_running = 0;
	gui_draw_bar();
}

void gui_shutdown_bar(void) {
	window_free(bar_window);
}
