#include "mms.h"
#include "defs.h"
#include "struct.h"
#include "extern.h"
#include "proto.h"

#define BETWEEN(n,start,size) (n > start+1 && n < start+size)

/* caller must check that the mouse click is in window range */
static void gpm_process_click(Window *win, int click, int mouse_y, int start_y)
{
	flist *ftmp;
	wlist *wlist;
	int i, was_active;

	if (!(win->flags & W_LIST))
		return;
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
				wlist->selected = delete_selected(wlist);
				win->update(win);
				doupdate();
			}
		}
		else
			process_return(wlist);
	}
	else {
		wlist->selected->flags &= ~F_SELECTED;
		wlist->selected = ftmp;
		wlist->selected->flags |= F_SELECTED;
		for (i = 0, ftmp = wlist->head; ftmp != wlist->selected; i++)
			ftmp = ftmp->next;
		wlist->where = i+1;
		info->update(win);
		win->update(win);
		doupdate();
	}
}

static void gpm_process_scroll(Window *win, int fakekey)
{
	change_active(win);
	if (info->update(move_selector(win, fakekey))) {
		win->update(win);
		doupdate();
	}
}

static int my_gpm_handler(Gpm_Event *e, void *data)
{
	if (!(e->type & GPM_DOWN))
		return 1;
	
	if (BETWEEN(e->x,files_x,files_width) && BETWEEN(e->y,files_y,files_height))
		gpm_process_click(files, e->buttons, e->y, files_y);
	else if (BETWEEN(e->x,play_x,play_width) && BETWEEN(e->y,play_y,play_height))
		gpm_process_click(play, e->buttons, e->y, play_y);
	else if (e->x == files_x+files_width && e->y == files_y+1)
		gpm_process_scroll(files, KEY_UP);
	else if (e->x == files_x+files_width && e->y == files_y+files_height)
		gpm_process_scroll(files, KEY_DOWN);
	else if (e->x == play_x+play_width && e->y == play_y+1)
		gpm_process_scroll(play, KEY_UP);
	else if (e->x == play_x+play_width && e->y == play_y+play_height)
		gpm_process_scroll(play, KEY_DOWN);

	/* else if scrollbars, etc */
	else
		return 1;

	return 1;
}

void gpm_init(void)
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

void gpm_close(void)
{
	Gpm_Close();
}
