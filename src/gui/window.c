#include "mjs.h"
#include "window.h"
#include "songdata/songdata.h"

#include "gui.h"

#include <string.h>

Window *window_new(void) {
	Window *window = calloc(1, sizeof (Window));

	return window;
}

int window_color(Window *window) {
	switch (window->name) {
		case WINDOW_NAME_FILELIST:
			return conf->colors[FILE_WINDOW];
		case WINDOW_NAME_INFO:
			return conf->colors[INFO_WINDOW];
		case WINDOW_NAME_BAR:
			return conf->colors[MENU_WINDOW];
		case WINDOW_NAME_PLAYBACK:
			return conf->colors[PLAYBACK_WINDOW];
		case WINDOW_NAME_PLAYLIST:
			return conf->colors[PLAY_WINDOW];
		default:
			return 0;
	}
}

int window_color_list_item(Window *window, songdata *list, songdata_song *ftmp) {
	int color;

	if (window->name == WINDOW_NAME_PLAYLIST) {
		if (ftmp == list->playing) {
			if ((ftmp == list->selected) && window->active)
				color = conf->colors[PLAY_SELECTED_PLAYING];
			else
				color = conf->colors[PLAY_UNSELECTED_PLAYING];
		} else if ((ftmp == list->selected) && window->active) {
			color = conf->colors[PLAY_SELECTED];
		} else {
			color = conf->colors[PLAY_UNSELECTED];
		}
	} else { // window->name == window_files
		if (ftmp->flags & (F_DIR | F_PLAYLIST)) {
			if ((ftmp == list->selected) && window->active)
				color = conf->colors[FILE_SELECTED_DIRECTORY];
			else
				color = conf->colors[FILE_UNSELECTED_DIRECTORY];
		} else if ((ftmp == list->selected) && window->active) {
			color = conf->colors[FILE_SELECTED];
		} else {
			color = conf->colors[FILE_UNSELECTED];
		}

		if (ftmp->flags & F_PAUSED) {
			color |= A_BLINK;
		}
	}

	return color;
}

void window_init(Window *window, WindowName name, WindowConfig *winconf) {
	window->name = name;

	window->x = winconf->x;
	window->y = winconf->y;
	window->height = winconf->height;
	window->width = winconf->width;
	window->title_dfl = winconf->title_dfl;
	window->title_fmt = winconf->title_fmt;
	window->format = winconf->format;

	window->win = newwin(window->height, window->width, window->y, window->x);
	window->panel = new_panel(window->win);

	wbkgd(window->win, window_color(window));
}

void window_activate(Window *window) {
	window->active = 1;
	window->update();
	top_panel(window->panel);
	update_panels();
}

void window_deactivate(Window *window) {
	window->active = 0;
	window->update();
	update_panels();
}

void window_input_list(Window *window, int c) {
	//TODO: find better workaround to keep GCC happy.
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"	
	int j, maxx, maxy, length;
	songdata_song *file;

	getmaxyx(window->win, maxy, maxx);
	length = maxy - 1;

#pragma GCC diagnostic pop	
	if (!window->list->selected) {
		return;
	}

	switch (c) {
		case KEY_HOME:
			window->list->selected = songdata_next_valid(window->list, window->list->head, c);
			window->list->where = 1;
			window->list->wheretop = 0;
			break;
		case KEY_END:
			window->list->selected = songdata_next_valid(window->list, window->list->tail, c);
			window->list->where = window->list->length;
			break;
		case KEY_DOWN:
			if ((file = songdata_next_valid(window->list, window->list->selected->next, c))) {
				window->list->selected = file;
				window->list->where++;
			}
			break;
		case KEY_UP:
			if ((file = songdata_next_valid(window->list, window->list->selected->prev, c))) {
				window->list->selected = file;
				window->list->where--;
			}
			break;
		case KEY_NPAGE:
			for (j = 0; window->list->selected->next && j < length - 1; j++) {
				window->list->selected = songdata_next_valid(window->list, window->list->selected->next, KEY_DOWN);
				window->list->where++;
			}
			window->list->selected = songdata_next_valid(window->list, window->list->selected, c);
			break;
		case KEY_PPAGE:
			for (j = 0; window->list->selected->prev && j < length - 1; j++) {
				window->list->selected = songdata_next_valid(window->list, window->list->selected->prev, KEY_UP);
				window->list->where--;
			}
			window->list->selected = songdata_next_valid(window->list, window->list->selected, c);
			break;
	}
}

void window_input(Window *window, int c) {
	if (window->type == WINDOW_TYPE_LIST) {
		window_input_list(window, c);
		window->update();
		gui_update_info();
	}
}

char *window_parse_tokens(Window *window, songdata_song *file, char *line,
		int size, const char *fmt) {
	int len = 0;
	char *artist;

	if (!(fmt) || !(file))
		return line;

	/* check for existence of these, set default values if necessary */
	if (file->artist)
		artist = file->artist;
	else
		artist = "Unknown";

	while (*fmt && (len < size)) {
		if (*fmt == '%') {
			switch (*++fmt) {
				case 't': /* the song title */
					strncat(line, file->filename, size - len);
					len += strlen(file->filename);
					break;
				case 'a': /* the artist */
					strncat(line, artist, size - len);
					len += strlen(artist);
					break;
				case 'f': /* filename */
					strncat(line, file->filename, size - len);
					len += strlen(file->filename);
					break;
				case 'F': /* complete filename, incl path */
					strncat(line, file->fullpath, size - len);
					len += strlen(file->fullpath);
					break;
				case 'p': /* the path */
					strncat(line, file->fullpath, size - len);
					len += strlen(file->fullpath);
					break;
				case 'l': /* the actual place and length of the playlist. */
					len += snprintf(line + len, size - len, "(%d/", window->list->where);
					len += snprintf(line + len, size - len, "%d)", window->list->length);
					break;
				case 'P': /* the path without the conf->mp3path part */
					if (file->flags & F_DIR) {
						strncat(line, file->relpath, size - len);
						len += strlen(file->relpath);
					} else {
						strncat(line, file->path, size - len);
						len += strlen(file->path);
					}
					break;
				case '%':
					line[len++] = '%';
					break;
				default:
					break;
			}
			fmt++;
		} else {
			line[len++] = *fmt++;
		}
	}
	return line;
}

char *window_parse_title(Window *window, char *title, int len) {
	char *p = window->title_dfl;

	if (window->title_fmt) {
		if (window->type == WINDOW_TYPE_LIST && window->list && window->list->selected) {
			p = window_parse_tokens(window, window->list->selected, title, len, window->title_fmt);
		} else if (window->name == WINDOW_NAME_INFO) {
			p = window_parse_tokens(window, window->file, title, len, window->title_fmt);
		}
	}

	return p;
}

void window_draw_title(Window *window) {
	WINDOW *win = window->win;
	char title[BUFFER_SIZE + 1], *p = NULL;
	int color;
	int i = 0, offset, x = window->width - 2;

	if (window->active) {
		color = conf->colors[WIN_ACTIVE_TITLE];
	} else {
		color = conf->colors[WIN_INACTIVE_TITLE];
	}

	memset(title, 0, sizeof (title));
	p = window_parse_title(window, title, BUFFER_SIZE);
	i = strlen(p);

	if (i == 0) {
		p = (char *) window->title_dfl;
		i = strlen(p);
	}

	if (i + 4 > x) {
		p[x - 4] = '\0';
		i = x - 4;
	}

	i += 4;

	offset = (x - i) / 2;

	wattrset(win, color);
	mvwprintw(win, 0, 1 + offset, "[ %s ]", p);
}

void window_draw_border(Window *window) {
	if (window->active) {
		wattrset(window->win, conf->colors[WIN_ACTIVE]);
	} else {
		wattrset(window->win, conf->colors[WIN_INACTIVE]);
	}
	box(window->win, 0, 0);
}

void window_draw_scrollbar(Window *window) {
	int x, y; /* window dimensions, etc */
	int top, bar; /* scrollbar portions */
	double value; /* how much each notch represents */
	int color, barcolor;
	songdata *list = window->list;
	WINDOW *win = window->win;

	getmaxyx(win, y, x);
	y -= 2;
	x -= 1;

	if (list->length < y + 1)
		return;

	value = list->length / (double) y;

	/* calculate the sizes of our status bar */
	top = (int) (list->wheretop / value + 0.5);
	bar = (int) (y / value + .5);

	if (bar < 1)
		bar = 1;

	/* because of rounding we may end up with too much, correct for that */
	if (top + bar > y)
		bar = y - top;

	if (window->active) {
		color = conf->colors[WIN_ACTIVE_SCROLL];
		barcolor = conf->colors[WIN_ACTIVE_SCROLLBAR];
	} else {
		color = conf->colors[WIN_INACTIVE_SCROLL];
		barcolor = conf->colors[WIN_INACTIVE_SCROLLBAR];
	}

	wattrset(win, color);
	mvwvline(win, 1, x, ACS_CKBOARD, y);
	wattrset(win, barcolor);
	mvwvline(win, 1 + top, x, ACS_CKBOARD, bar);

	update_panels();
}

void window_draw_info(Window *window) {
	WINDOW *win = window->win;
	songdata_song *file = window->file;

	char *title = "";
	char *artist = "";
	char *album = "";
	char *genre = "";

	werase(win);

	if (file != NULL) {
		if (file->flags & F_DIR) {
			title = "(Directory)";
		} else {
			title = file->title;
			artist = file->artist ? file->artist : "Unknown";
			album = file->album ? file->album : "Unknown";
			genre = file->genre ? file->genre : "Unknown";
		}
	}

	wattrset(win, conf->colors[INFO_TEXT]);
	mvwprintw(win, 1 + window->yoffset, 2, "Title : %s", title);
	mvwprintw(win, 2 + window->yoffset, 2, "Artist: %s", artist);
	mvwprintw(win, 3 + window->yoffset, 2, "Album : %s", album);
	if (conf->c_flags & C_USE_GENRE) {
		mvwprintw(win, 4 + window->yoffset, 2, "Genre : %s", genre);
	}

	window_draw_border(window);
	window_draw_title(window);
}

void window_draw_list(Window *window) {
	int x = window->width - 4, y = window->height - 2, i;
	char buf[BUFFER_SIZE + 1];
	char *line;
	int color;
	WINDOW *win = window->win;
	songdata_song *ftmp;
	songdata *list = window->list;

	if (!list)
		return;

	// do we need to reposition the screen
	if (list->length > y - 1) {
		if ((list->where - 4) == list->wheretop) {
			list->wheretop--;
			if (list->wheretop < 0)
				list->wheretop = 0;
		} else if ((list->where + 3) == (list->wheretop + y)) {
			if (++list->wheretop > (list->length - y))
				list->wheretop = list->length - y;
		} else if (((list->where - 5) < list->wheretop) || ((list->where + 3) > (list->wheretop + y))) {
			list->wheretop = list->where - (y / 2);
			if (list->wheretop < 0)
				list->wheretop = 0;
			if (list->wheretop > (list->length - y))
				list->wheretop = list->length - y;
		}
	}
	// find the list->top entry
	list->top = list->head;
	if ((list->wheretop > 0) & (list->length > y)) {
		for (i = 0; (i < list->wheretop) && (list->top->next); i++)
			list->top = list->top->next;
	}

	werase(win);

	ftmp = list->top;
	for (i = 0; i < y && ftmp; i++) {
		if (window->format) {
			memset(buf, 0, sizeof ( buf));
			line = window_parse_tokens(window, ftmp, buf, BUFFER_SIZE, window->format);
		} else {
			line = ftmp->filename;
		}

		color = window_color_list_item(window, list, ftmp);
		wattrset(win, color);
		mvwaddnstr(win, i + 1, 2, line, x);

		ftmp = ftmp->next;
	}

	window_draw_border(window);
	window_draw_scrollbar(window);
	window_draw_title(window);
}

void window_update(Window *window) {
	wnoutrefresh(window->win);
}

void window_free(Window *window) {
	if (window->panel) {
		del_panel(window->panel);
	}
	if (window->win) {
		delwin(window->win);
	}
	free(window);
}
