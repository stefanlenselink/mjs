#include "top.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "window.h"
#include "misc.h"
#include "tokens.h"
#include "files.h"
#include "extern.h"

static void	 clear_info(void);
static u_char	*parse_title(Window *, u_char *, int);

int
show_list(Window *window)
{
	int x = window->width-4, y = window->height-1, i;
	u_char buf[BUFFER_SIZE+1];
	const u_char *line;
	WINDOW *win = window->win;
	flist *ftmp;

	if (!window->contents.list)
		return 0;
	ftmp = window->contents.list->top;

	for (i = 1; i < y; i++) {
		if (ftmp && *ftmp->filename) {
			if (window->format) {
				memset(buf, 0, sizeof(buf));
				line = parse_tokens(ftmp, buf, BUFFER_SIZE, window->format);
			} else {
				line = ftmp->filename;
			}
			if ((window->flags & W_ACTIVE) && (ftmp->flags & F_PAUSED))
				my_mvwnaddstr(win, i, 2, ftmp->colors | A_BLINK, x, line);
			else
				my_mvwnaddstr(win, i, 2, ftmp->colors, x, line);
			ftmp = ftmp->next;
		} else /* blank the line */
			my_mvwnaddstr(win, i, 2, colors[FILE_BACK], x, "");
	}
	if (active->flags & W_LIST)
		do_scrollbar(active);
	update_title(window);
	update_panels();
	return 1;
}

Window *
move_selector(Window *window, int c)
{
	flist *ftmp, *file, *list;
	wlist *wlist = window->contents.list;
	int i, j, maxx, maxy, length;
	
	if (!wlist)
		return NULL;
	
	getmaxyx(window->win, maxy, maxx);
	length = maxy - 2;
	maxy -= 3;

	/* check these two cases here because we can avoid the for() loop below */
	if (c == KEY_HOME) {
		wlist->selected->flags &= ~F_SELECTED;
		if (wlist->selected->flags & F_PLAY)
			wlist->selected->colors = colors[PLAYING];
		else
			wlist->selected->colors = colors[UNSELECTED];
		wlist->top = wlist->selected = next_valid(window, wlist->head, c);
		wlist->selected->flags |= F_SELECTED;
		if (wlist->selected->flags & F_PLAY)
			wlist->selected->colors = colors[SEL_PLAYING];
		else
			wlist->selected->colors = colors[SELECTED];
		wlist->where = 1;
		return window;
	} else if (c == KEY_END) {
		wlist->selected->flags &= ~F_SELECTED;
		if (wlist->selected->flags & F_PLAY)
			wlist->selected->colors = colors[PLAYING];
		else
			wlist->selected->colors = colors[UNSELECTED];
		wlist->selected = next_valid(window, wlist->tail, c);
		wlist->selected->flags |= F_SELECTED;
		if (wlist->selected->flags & F_PLAY)
			wlist->selected->colors = colors[SEL_PLAYING];
		else
			wlist->selected->colors = colors[SELECTED];
		wlist->where = wlist->length;
		if (length < wlist->length) {
			for (i = 1, ftmp = wlist->tail; ftmp && (i < length); i++)
				ftmp = ftmp->prev;
			wlist->top = ftmp;
		} else
			wlist->top = wlist->head;
		return window;
	}
	ftmp = list = wlist->top;
	/* count the distance from the top to selected... only a few loops */
	for (i = 0; ftmp && !(ftmp->flags & F_SELECTED) && (i < maxy); i++)
		ftmp = ftmp->next;
	if (!ftmp)
		return NULL;
	switch (c) {
		case KEY_DOWN:
			if ((file = next_valid(window, ftmp->next, c))) {
				ftmp->flags &= ~F_SELECTED;
				if (ftmp->flags & F_PLAY)
					ftmp->colors = colors[PLAYING];
				else
					ftmp->colors = colors[UNSELECTED];
				file->flags |= F_SELECTED;
				if (file->flags & F_PLAY)
					file->colors = colors[SEL_PLAYING];
				else
					file->colors = colors[SELECTED];
				wlist->selected = file;
				wlist->where++;
				if (i == maxy)
					wlist->top = list->next;
				return window;
			}
			break;
		case KEY_UP:
			if ((file = next_valid(window, ftmp->prev, c))) {
				ftmp->flags &= ~F_SELECTED;
				if (ftmp->flags & F_PLAY)
					ftmp->colors = colors[PLAYING];
				else
					ftmp->colors = colors[UNSELECTED];
				file->flags |= F_SELECTED;
				if (file->flags & F_PLAY)
					file->colors = colors[SEL_PLAYING];
				else
					file->colors = colors[SELECTED];
				wlist->selected = file;
				wlist->where--;
				if (i == 0)
					wlist->top = list->prev;
				return window;
			}
			break;
		case KEY_NPAGE:
			ftmp->flags &= ~F_SELECTED;
			if (ftmp->flags & F_PLAY)
				ftmp->colors = colors[PLAYING];
			else
				ftmp->colors = colors[UNSELECTED];
			for (j = 0; ftmp->next && j < length; j++) {
				ftmp = ftmp->next;
				wlist->where++;
			}
			ftmp = next_valid(window, ftmp, c);
			if (i+j > maxy)
				for (i = 0; i < j; i++)
					list = list->next;
			wlist->top = list;
			wlist->selected = ftmp;
			ftmp->flags |= F_SELECTED;
			if (ftmp->flags & F_PLAY)
				ftmp->colors = colors[SEL_PLAYING];
			else
				ftmp->colors = colors[SELECTED];
			return window;
		case KEY_PPAGE:
			ftmp->flags &= ~F_SELECTED;
			if (ftmp->flags & F_PLAY)
				ftmp->colors = colors[PLAYING];
			else
				ftmp->colors = colors[UNSELECTED];
			for (j = 0; ftmp->prev && j < length; j++) {
				wlist->where--;
				ftmp = ftmp->prev;
				if (list->prev)
					list = list->prev;
			}
			ftmp = next_valid(window, ftmp, c);
			wlist->top = list;
			wlist->selected = ftmp;
			ftmp->flags |= F_SELECTED;
			if (ftmp->flags & F_PLAY)
				ftmp->colors = colors[SEL_PLAYING];
			else
				ftmp->colors = colors[SELECTED];
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
		my_mvwnprintw(win, 1, 12, colors[UNSELECTED], i-13, "%s", (file->flags & F_DIR) ? "(Directory)" : file->filename);
		if (file->artist)
			my_mvwnprintw(win, 2, 12, colors[UNSELECTED], i-13, "%s", file->artist);
		if (file->title)
			my_mvwnprintw(win, 3, 12, colors[UNSELECTED], i-13, "%s", file->title);
		if (!(file->flags & F_DIR)) {
			u_char length[7];
			memset(length, 0, sizeof(length));
			snprintf(length, 7, "%lu:%02lu", file->length/60, file->length%60);
		 	my_mvwprintw(win, 4, 12, colors[UNSELECTED], "%-6s Genre: %-13s   Bitrate: %-3d", length, file->genre? : "Unknown", file->bitrate);
		}
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

static void
clear_info(void)
{
	WINDOW *win = info->win;
	int i = info->width;
	
	my_mvwnclear(win, 1, 12, i-13);
	my_mvwnclear(win, 2, 12, i-13);
	my_mvwnclear(win, 3, 12, i-13);
	my_mvwnclear(win, 4, 12, 7);
	my_mvwnclear(win, 4, 26, 12);
	my_mvwnclear(win, 4, 50, i-51);
}

int
active_win(Window *window)
{
	WINDOW *win = window->win;
	PANEL *panel = window->panel;

	int i, x, y;
	wborder(win, 'º', 'º', 'Í', 'Í', 'É', '»', 'È', '¼');
	getmaxyx(win, y, x);
	mvwchgat(win, 0, 0, x, A_ALTCHARSET | colors[ACTIVE], 0, NULL);
	mvwchgat(win, y-1, 0, x, A_ALTCHARSET | colors[ACTIVE], 0, NULL);
	for (i = 0; i < y; i++) {
		mvwchgat(win, i, 0, 1, A_ALTCHARSET | colors[ACTIVE], 0, NULL);
		mvwchgat(win, i, x-1, 1, A_ALTCHARSET | colors[ACTIVE], 0, NULL);
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
	mvwchgat(win, 0, 0, x, A_ALTCHARSET | colors[INACTIVE], 0, NULL);
	mvwchgat(win, y-1, 0, x, A_ALTCHARSET | colors[INACTIVE], 0, NULL);
	for (i = 0; i < y; i++) {
		mvwchgat(win, i, 0, 1, A_ALTCHARSET | colors[INACTIVE], 0, NULL);
		mvwchgat(win, i, x-1, 1, A_ALTCHARSET | colors[INACTIVE], 0, NULL);
	}	
	window->flags &= ~W_ACTIVE;
	update_title(window);
	update_panels();
	return 1;
}

int
inactive_edit(Window *window)
{
	/* this is kind of stupid but its how its gotta be! */
	return hide_panel(window->panel);
}

int
update_title(Window *window)
{
	WINDOW *win = window->win;
	u_char title[BUFFER_SIZE+1], *p = NULL, horiz = ACS_HLINE;
	u_int32_t color = colors[INACTIVE];
	int i = 0, left, right, center, x = window->width-2;

	if (window->flags & W_ACTIVE)
		horiz = 'Í', color = colors[ACTIVE];

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
		my_mvwprintw(win, 0, 1+center, colors[TITLE], "[ %s ]", p);
		mvwhline(win, 0, 1+center+i, horiz | A_ALTCHARSET | color, right);
		return 1;
	}
	return 0;
}

void
do_scrollbar(Window *window)
{
	int i = 1, offscreen, x, y; /* window dimensions, etc */
	int top, bar, bottom; /* scrollbar portions */
	double value; /* how much each notch represents */
	wlist *wtmp = window->contents.list;
	flist *ftmp = NULL, *selected = wtmp->selected;
	WINDOW *win = window->win;
	if (wtmp->length < 1)
		return;
	getmaxyx(win, y, x);
	y -= 2;
	x -= 1;
	value = wtmp->length / (double) y;
	for (ftmp = wtmp->top; ftmp != selected; ftmp = ftmp->next)
		i++;
	/* now we calculate the number preceeding the screenfull */
	offscreen = wtmp->where - i;

	/* calculate the sizes of our status bar */
	if (value < 1) {
		top = 0;
		bar = y;
		bottom = 0;
	} else {
		/* drago's code */
		double toptmp;
		toptmp=offscreen / value;
		top = (int)(double)(int)toptmp/1 == toptmp ? toptmp : (double)(int)toptmp+(double)1;
		bar = (int)((y / value)+(double).5);
		bottom = y - top - bar;
		/* end drago's code =) */
	}
	/* because of rounding we may end up with too much, correct for that */
	if (bottom < 0)
		top += bottom;
	mvwvline(win, 1, x, ACS_CKBOARD | A_ALTCHARSET | colors[SCROLL], top);
	mvwvline(win, 1+top, x, ACS_BLOCK | A_ALTCHARSET | colors[SCROLLBAR], bar);
	if (bottom > 0)
		mvwvline(win, 1+top+bar, x, ACS_CKBOARD | A_ALTCHARSET | colors[SCROLL], bottom);
#ifdef GPM_SUPPORT
	mvwaddch(win, 0, x, ACS_UARROW | A_ALTCHARSET | colors[ARROWS]);
	mvwaddch(win, y+1, x, ACS_DARROW | A_ALTCHARSET | colors[ARROWS]);
#endif
	update_panels();
}

u_char *
parse_title(Window *win, u_char *title, int len)
{
	u_char *p = (u_char *)win->title_dfl;
		
	if (win->title_fmt) {
		if (win->flags & W_LIST && win->contents.list && win->contents.list->selected)
			p = (u_char *)parse_tokens(win->contents.list->selected, title, len, win->title_fmt);
		else if (!(win->flags & W_LIST) && win->contents.play)
			p = (u_char *)parse_tokens(win->contents.play, title, len, win->title_fmt);
		else if (info->contents.play)
			p = (u_char *)parse_tokens(info->contents.play, title, len, win->title_fmt);
	}

	return p;
}
