#include "top.h"
#include "defs.h"
#include "struct.h"
#include "extern.h"
#include "mms_gpm.h"
#include "misc.h"


static void	gpm_process_click(Window *, int, int, int);
static void	gpm_process_arrows(Window *, int);
static void	gpm_process_scroll(Window *, int, int, int);
static int	my_gpm_handler(Gpm_Event *, void *);

/* caller must check that the mouse click is in window range */
static void
gpm_process_click(Window *win, int click, int mouse_y, int start_y)
{
	flist *ftmp;
	wlist *wlist;
	int i, was_active;

	/* already checked in my_gpm_handler
	if (!(win->flags & W_LIST))
		return;
	*/
	wlist = win->contents.list;
	if (!wlist || wlist->length == 0)
		return;
	
	was_active = (active == win);
	
	change_active(win);
	ftmp = wlist->top;
	for (i = 0; ftmp->next && i < mouse_y - (start_y+2); i++)
		ftmp = ftmp->next;
	if (was_active && ftmp->flags & F_SELECTED) {
		if (win == play && click & GPM_B_RIGHT) {
			if (!(active->flags & W_RDONLY)) {
				delete_file(active, wlist->selected);
				win->update(win);
				info->update(info);
				doupdate();
			}
		}
		else
			process_return(wlist);
	}
	else {
		wlist->selected->flags &= ~F_SELECTED;
		if (wlist->selected->flags & F_PLAY)
			wlist->selected->colors = colors[PLAYING];
		else
			wlist->selected->colors = colors[UNSELECTED];
		wlist->selected = ftmp;
		ftmp->flags |= F_SELECTED;
		if (ftmp->flags & F_PLAY)
			ftmp->colors = colors[SEL_PLAYING];
		else
			ftmp->colors = colors[SELECTED];
		for (i = 0, ftmp = wlist->head; ftmp != wlist->selected; i++)
			ftmp = ftmp->next;
		wlist->where = i+1;
		info->update(win);
		win->update(win);
		doupdate();
	}
}

/* process clicks on the arrows above and below scrollbars */
static void
gpm_process_arrows(Window *win, int fakekey)
{
	change_active(win);
	if (info->update(move_selector(win, fakekey))) {
		win->update(win);
		doupdate();
	}
}

/* process clicks on the scrollbars */
static void
gpm_process_scroll(Window *win, int click, int mouse_y, int start_y)
{
	/* from do_scrollbar in misc.c */
	int i = 1, offscreen, x = win->width-2, y = win->height-1, key = -1;
	int top, bar, bottom;
	double value;
	wlist *wtmp = win->contents.list;
	flist *ftmp = NULL, *selected = wtmp->selected;

	change_active(win);

	if (wtmp->length < 1)
		return;

	value = wtmp->length / (double) y;
	for (ftmp = wtmp->top; ftmp != selected; ftmp = ftmp->next)
		i++;
	offscreen = wtmp->where - i;

	if (value < 1) {
		top = 0;
		bar = y;
		bottom = 0;
	} else {
		double toptmp;
		toptmp=offscreen / value;
		top = (int) (double)(int)toptmp/1 == toptmp ? toptmp : (double)(int)toptmp+(double)1;
		bar = (int)((y / value)+(double).5);
		bottom = y - top - bar;
	}
	if (bottom < 0)
		top += bottom;

	if (click & GPM_B_LEFT) {
		if (BETWEEN(mouse_y, start_y, top+2))
			key = KEY_PPAGE;
		else if (BETWEEN(mouse_y, start_y+top+bar, y+2))
			key = KEY_NPAGE;
	}
	else {
		if (BETWEEN(mouse_y, start_y, top+2))
			key = KEY_HOME;
		else if (BETWEEN(mouse_y, start_y+top+bar, y+2))
			key = KEY_END;
	}
	if (key == -1)
		return;
	
	if (info->update(move_selector(win, key))) {
		win->update(win);
		doupdate();
	}
}

static int
my_gpm_handler(Gpm_Event *e, void *data)
{
	if (!(e->type & GPM_DOWN))
		return 1;
	
	if (BETWEEN(e->x,files->x,files->width) && BETWEEN(e->y,files->y,files->height))
		gpm_process_click(files, e->buttons, e->y, files->y);
	else if (BETWEEN(e->x,play->x,play->width) && BETWEEN(e->y,play->y,play->height))
		gpm_process_click(play, e->buttons, e->y, play->y);
	else if (e->x == files->x+files->width && e->y == files->y+1)
		gpm_process_arrows(files, KEY_UP);
	else if (e->x == files->x+files->width && e->y == files->y+files->height)
		gpm_process_arrows(files, KEY_DOWN);
	else if (e->x == play->x+play->width && e->y == play->y+1)
		gpm_process_arrows(play, KEY_UP);
	else if (e->x == play->x+play->width && e->y == play->y+play->height)
		gpm_process_arrows(play, KEY_DOWN);
	else if (e->x == files->x+files->width && BETWEEN(e->y, files->y, files->y+files->height))
		gpm_process_scroll(files, e->buttons, e->y, files->y);
	else if (e->x == play->x+play->width && BETWEEN(e->y, play->y, play->y+play->height))
		gpm_process_scroll(play, e->buttons, e->y, play->y);
	else
		return 1;

	return 1;
}

void
gpm_init(void)
{
	Gpm_Connect connect;

	connect.eventMask = GPM_DOWN;
	connect.defaultMask = ~GPM_DOWN;
	connect.maxMod = ~0;
	connect.minMod = 0;

	if (Gpm_Open(&connect, 0) == -1)
		return;

	gpm_handler = my_gpm_handler;
	gpm_visiblepointer = 1;
}

void
gpm_close(void)
{
	Gpm_Close();
}
