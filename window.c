#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "window.h"
#include "misc.h"
#include "tokens.h"
#include "files.h"
#include "extern.h"

static u_char	*parse_title(Window *, u_char *, int);

int
show_list(Window *window)
{
	int x = window->width-4, y = window->height-2, i;
	u_char buf[BUFFER_SIZE+1];
	const u_char *line;
	u_int32_t color;
	WINDOW *win = window->win;
	flist *ftmp;
	wlist *list = window->contents.list;

	if (!list)
		return 0;

	// do we need to reposition the screen
	if ((list->where - 3) == list->wheretop) {
		list->wheretop--;
		if (list->wheretop < 0)
			list->wheretop = 0;
	} else if ((list->where + 4) == (list->wheretop + y)) {
		if (++list->wheretop > (list->length - y))
			list->wheretop = list->length - y;
	} else if (((list->where - 4) < list->wheretop) || ((list->where + 4) > (list->wheretop + y))) {
		list->wheretop = list->where - (y/2);
		if (list->wheretop <  0)
			list->wheretop = 0;
		if (list->wheretop > (list->length - y))
			list->wheretop = list->length - y;
	}

	// find the list->top entry
	list->top=list->head;
	if ((list->wheretop > 0) & (list->length > y)) {
		for (i = 0; (i < list->wheretop) && (list->top->next); i++)
			list->top = list->top->next;
	}

	ftmp = list->top;
	for (i = 0; i < y; i++)
		if (ftmp) {
			if (window->format) {
				memset(buf, 0, sizeof(buf));
				line = parse_tokens(window,ftmp, buf, BUFFER_SIZE, window->format);
			} else 	
				line = ftmp->filename;
			if (window==play) {
				if (ftmp->flags & F_PLAY)
					if ((ftmp->flags & F_SELECTED) && (window->flags & W_ACTIVE))
						color = colors[PLAY_SELECTED_PLAYING];
					else
						color = colors[PLAY_UNSELECTED_PLAYING];
				else			
					if ((ftmp->flags & F_SELECTED) && (window->flags & W_ACTIVE))
						color = colors[PLAY_SELECTED];
					else
						color = colors[PLAY_UNSELECTED];
			} else	// window==files
				if (ftmp->flags & F_DIR)
					if ((ftmp->flags & F_SELECTED) && (window->flags & W_ACTIVE))
						color = colors[FILE_SELECTED_DIRECTORY];
					else
						color = colors[FILE_UNSELECTED_DIRECTORY];
				else
					if ((ftmp->flags & F_SELECTED) && (window->flags & W_ACTIVE))
						color = colors[FILE_SELECTED];
					else 	
						color = colors[FILE_UNSELECTED];
			if (ftmp->flags & F_PAUSED)
				color |=  A_BLINK;
			my_mvwnaddstr(win, (i+1), 2, color, x, line);
			ftmp = ftmp->next;
		} else  /* blank the line */ 
			if (window == play) 
				my_mvwnaddstr(win, (i+1), 2, colors[PLAY_WINDOW], x, "");
			else
				my_mvwnaddstr(win, (i+1), 2, colors[FILE_WINDOW], x, "");

	if (window->flags & W_LIST)
		do_scrollbar(active);
	update_title(window);
	update_panels();
	if (conf->c_flags & C_FIX_BORDERS)
		redrawwin(win);
	return 1;
}

Window *
move_selector(Window *window, int c)
{
	flist *file;
	wlist *list = window->contents.list;
	int j, maxx, maxy, length;
	
	if (!list)
		return NULL;
	if (!list->selected)
		return NULL;
	
	getmaxyx(window->win, maxy, maxx);
	length = maxy - 1;

	switch (c) {
		case KEY_ENTER:
		case '\n':
		case '\r':
			if (active == play) {
				list->selected->flags &= ~F_SELECTED;
				list->selected = next_valid(list, list->head, KEY_HOME);
				list->where = 0;
				list->wheretop = 0;
				
				for (j = 0; list->selected->next && list->selected != list->playing; j++) {
					list->selected = next_valid(list, list->selected->next, KEY_DOWN);
					list->where++;
				}
				list->selected->flags |= F_SELECTED;
				return window;
			}
			break;				
		case KEY_HOME: 
			list->selected->flags &= ~F_SELECTED;
			list->selected = next_valid(list, list->head, c);
			list->selected->flags |= F_SELECTED;
			list->where = 0;
			list->wheretop = 0;
			return window;
		case KEY_END: 
			list->selected->flags &= ~F_SELECTED;
			list->selected = next_valid(list, list->tail, c);
			list->selected->flags |= F_SELECTED;
			list->where = list->length;
			return window;
		case KEY_DOWN:
			if ((file = next_valid(list, list->selected->next, c))) {
				list->selected->flags &= ~F_SELECTED;
				file->flags |= F_SELECTED;
				list->selected = file;
				list->where++;
				return window;
			}
			break;
		case KEY_UP:
			if ((file = next_valid(list, list->selected->prev, c))) {
				list->selected->flags &= ~F_SELECTED;
				file->flags |= F_SELECTED;
				list->selected = file;
				list->where--;
				return window;
			}
			break;
		case KEY_NPAGE:
			list->selected->flags &= ~F_SELECTED;
			for (j = 0; list->selected->next && j < length-1; j++) {
				list->selected = next_valid(list, list->selected->next, KEY_DOWN);
				list->where++;
				}
			list->selected = next_valid(list, list->selected, c);
			list->selected->flags |= F_SELECTED;
			return window;
		case KEY_PPAGE:
			list->selected->flags &= ~F_SELECTED;
			for (j = 0; list->selected->prev && j < length-1; j++) {
				list->selected = next_valid(list, list->selected->prev, KEY_UP);
				list->where--;
				}
			list->selected = next_valid(list, list->selected, c);
			list->selected->flags |= F_SELECTED;
			return window;
		default:
			break;
	}
	return NULL;
}

int
update_info(Window *window)
{
	WINDOW *win = info->win;
	int i = info->width;
	flist *file = NULL;

	if (!window || !window->contents.list)
		return 0;

	if (window->flags & W_LIST)
		file = window->contents.list->selected;
	else
		file = window->contents.play;

	clear_info();

	if (file) {
		my_mvwnprintw(win, 1, 10, colors[INFO_TEXT], i-11, "%s", (file->flags & F_DIR) ? "(Directory)" : file->filename);
		if (file->artist)
			my_mvwnprintw(win, 2, 10, colors[INFO_TEXT], i-11, "%s", file->artist);
		if (file->album)
			my_mvwnprintw(win, 3, 10, colors[INFO_TEXT], i-11, "%s", file->album);
	}
	update_title(info);
	update_panels();
	return 1;
}

void change_active(Window *new)
{
	active->deactivate(active);
	active->update(active);
	active = new;
	active->activate(active);
	active->update(active);
	info->update(active);
	doupdate();
}

__inline__ void
clear_info(void)
{
	int i = info->height - 2;
	for (; i; i--)
		my_mvwnclear(info->win, i, 9, info->width - 10);
	update_panels();
	if (conf->c_flags & C_FIX_BORDERS)
		redrawwin(info->win);
}

int
active_win(Window *window)
{
	WINDOW *win = window->win;
	PANEL *panel = window->panel;

	int i, x, y;
	if (conf->c_flags & C_FIX_BORDERS)
		wborder(win, 0, 0, '=', '=', 0, 0, 0, 0);
	else
		wborder(win, 'º', 'º', 'Í', 'Í', 'É', '»', 'È', '¼');
	getmaxyx(win, y, x);
	mvwchgat(win, 0, 0, x, A_ALTCHARSET | colors[WIN_ACTIVE], 0, NULL);
	mvwchgat(win, y-1, 0, x, A_ALTCHARSET | colors[WIN_ACTIVE], 0, NULL);
	for (i = 0; i < y; i++) {
		mvwchgat(win, i, 0, 1, A_ALTCHARSET | colors[WIN_ACTIVE], 0, NULL);
		mvwchgat(win, i, x-1, 1, A_ALTCHARSET | colors[WIN_ACTIVE], 0, NULL);
	}	
	window->flags |= W_ACTIVE;
	update_title(window);
	top_panel(panel);
	update_panels();
	return 1;
}

int
inactive_win(Window *window)
{
	WINDOW *win = window->win;
	int i, x, y;
	wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
	getmaxyx(win, y, x);
	mvwchgat(win, 0, 0, x, A_ALTCHARSET | colors[WIN_INACTIVE], 0, NULL);
	mvwchgat(win, y-1, 0, x, A_ALTCHARSET | colors[WIN_INACTIVE], 0, NULL);
	for (i = 0; i < y; i++) {
		mvwchgat(win, i, 0, 1, A_ALTCHARSET | colors[WIN_INACTIVE], 0, NULL);
		mvwchgat(win, i, x-1, 1, A_ALTCHARSET | colors[WIN_INACTIVE], 0, NULL);
	}	
	window->flags &= ~W_ACTIVE;
	update_title(window);
	update_panels();
	return 1;
}

int 
clear_menubar(Window *window)
{
	wmove(window->win, 0, 0);
	wclrtoeol(window->win);
	wbkgd(window->win, colors[MENU_WINDOW]);
	return 1;
}

int
std_menubar(Window *window)
{
	char version_str[128];
	int x = window->width-2;
	clear_menubar(window);
	my_mvwaddstr(window->win, 0, ((x-strlen(window->title_dfl))/2), colors[MENU_TEXT], window->title_dfl);
	snprintf(version_str, 128, "v%s", VERSION);
	my_mvwaddstr(window->win, 0, x - strlen(version_str) + 2, colors[MENU_TEXT], version_str);
	return 1;
}

__inline__ void
printf_menubar(Window *window, char *text) 
{
	clear_menubar(window);
	my_mvwaddstr(window->win, 0, 10, colors[MENU_TEXT], text);
	update_panels();
	doupdate();

}

int
update_title(Window *window)
{
	WINDOW *win = window->win;
	u_char title[BUFFER_SIZE+1], *p = NULL, horiz = ACS_HLINE;
	u_int32_t color;
	int i = 0, left, right, center, x = window->width-2;

	if (window->flags & W_ACTIVE) {
		if (conf->c_flags & C_FIX_BORDERS)
			horiz = '=', color = colors[WIN_ACTIVE];
		else
			horiz = 'Í', color = colors[WIN_ACTIVE];
	} else
		color = colors[WIN_INACTIVE];

	memset(title, 0, sizeof(title));
	p = parse_title(window, title, BUFFER_SIZE);
	i = strlen(p)+4;

	if (i > x) { 
		if ((i = strlen(window->title_dfl)) < x) {
			i += 4;
			p = (u_char *)window->title_dfl;
		} else {
			p = "";
			i = 4;
		}
	}

	center = x-i-(x-i)/2;
	right = (x-i)/2;
	left = x-i-right;

	if (p && (i <= x)) {
		mvwhline(win, 0, 1, horiz | A_ALTCHARSET | color, left);
		if (window->flags & W_ACTIVE)
			my_mvwprintw(win, 0, 1+center, colors[WIN_ACTIVE_TITLE], "[ %s ]", p);
		else
			my_mvwprintw(win, 0, 1+center, colors[WIN_INACTIVE_TITLE], "[ %s ]", p);
		mvwhline(win, 0, 1+center+i, horiz | A_ALTCHARSET | color, right);
		return 1;
	}
	return 0;
}

void
do_scrollbar(Window *window)
{
	int x, y; /* window dimensions, etc */
	int top, bar, bottom; /* scrollbar portions */
	double value; /* how much each notch represents */
	u_int32_t color, barcolor;
	wlist *list = window->contents.list;
	WINDOW *win = window->win;

	getmaxyx(win, y, x);
	y -= 2;
	x -= 1;

	value = list->length / (double) y;

	/* calculate the sizes of our status bar */
	if (value < 1) {
		// only one screen
		top = 0;
		bar = y;
		bottom = 0;
	} else {
		top = (int)(((double)list->wheretop / value)+(double).5);
		bar = (int)((y / value)+(double).5);
		bottom = y - top - bar ;
	}

	/* because of rounding we may end up with too much, correct for that */
	if (bottom < 0)
		top += bottom;

	if (window->flags & W_ACTIVE) {
		color = colors[WIN_ACTIVE_SCROLL];
		barcolor = colors[WIN_ACTIVE_SCROLLBAR];
	} else {
		color = colors[WIN_INACTIVE_SCROLL];
		barcolor = colors[WIN_INACTIVE_SCROLLBAR];
	}

	mvwvline(win, 1, x, ACS_BOARD | A_ALTCHARSET | color, top);
	mvwvline(win, 1 + top, x, ACS_BLOCK | A_ALTCHARSET | barcolor, bar);
	if (bottom > 0)
		mvwvline(win, 1 + top + bar, x, ACS_BOARD | A_ALTCHARSET | color, bottom);

	update_panels();
}

u_char *
parse_title(Window *win, u_char *title, int len)
{
	u_char *p = (u_char *)win->title_dfl;
		
	if (win->title_fmt) {
		if (win->flags & W_LIST && win->contents.list && win->contents.list->selected)
			p = (u_char *)parse_tokens(win, win->contents.list->selected, title, len, win->title_fmt);
		else if (!(win->flags & W_LIST) && win->contents.play)
			p = (u_char *)parse_tokens(win, win->contents.play, title, len, win->title_fmt);
		else if (info->contents.play)
			p = (u_char *)parse_tokens(win, info->contents.play, title, len, win->title_fmt);
	}

	return p;
}
