#include "defs.h"
#include "mms.h"
#include "struct.h"
#include "proto.h"
#include "extern.h"

void
play_next_song (void)
{
	flist *ftmp = info->contents.play;

	if (!ftmp)
		return;

	ftmp->flags &= ~F_PLAY;
	if (conf->loop && !ftmp->next)
		ftmp = play->contents.list->head;
	else
		ftmp = ftmp->next;
	if (!jump_to_song(ftmp)) {
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		play->update(play);
		doupdate();
	}
}

wlist *
jump_forward (wlist *playlist)
{
	flist *ftmp = info->contents.play;

	if (!ftmp)
		return playlist;
	ftmp->flags &= ~F_PLAY;
	if (conf->loop && !ftmp->next)
		ftmp = play->contents.list->head;
	else
		ftmp = ftmp->next;
	if (!jump_to_song(ftmp)) {
		stop_player(playlist);
    info->contents.play = NULL;
		play->contents.list->playing = NULL;
		play->update(play);
	}
	return playlist;
}

wlist *
jump_backward (wlist *playlist)
{
	flist *ftmp = info->contents.play;

	if (!ftmp)
		return playlist;
	ftmp->flags &= ~F_PLAY;
	if (conf->loop && !ftmp->prev)
		ftmp = play->contents.list->tail;
	else
		ftmp = ftmp->prev;
	if (!jump_to_song(ftmp)) {
		stop_player(playlist);
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		show_list(play);
	}
	return playlist;
}

int
jump_to_song(flist *selected)
{
	char buf[BIG_BUFFER_SIZE+1];
	wlist *playlist = play->contents.list;
	
	if (!playlist || !selected)
		return 0;
	
	if (info->contents.play)
		info->contents.play->flags &= ~F_PLAY;

	memset(buf, 0, sizeof(buf));
	snprintf(buf, BIG_BUFFER_SIZE, "%s/%s", selected->path, selected->filename);
	send_cmd(inpipe[1], LOAD, buf);
	clear_play_info();
	p_status = 1;
	selected->flags |= F_PLAY;
	playlist->playing = selected;
	info->contents.play = selected;
	if (active == info)
		info->update(info);
	play->update(play);
	
	doupdate();
	return 1;
}

wlist *
stop_player (wlist *playlist)
{
	flist *ftmp = info->contents.play;
	
	if (ftmp) {
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		ftmp->flags &= ~F_PLAY;
		play->update(play);
	}
	send_cmd(inpipe[1], STOP);
	return playlist;
}

wlist *
pause_player (wlist *playlist)
{
	extern int p_status;
	if (p_status == 1)
		p_status = 2;
	else if (p_status == 2)
		p_status = 1;
	send_cmd(inpipe[1], PAUSE);
	return playlist;
}

flist *
delete_selected (wlist *playlist, flist *selected)
{
	flist *fnext, *fprev, *ftmp;
	int i, maxx, maxy;
	if (!playlist)
		return NULL;
	getmaxyx(play->win, maxy, maxx);
	maxy -= 3;
	if (!selected)
		return NULL;
	if (selected->flags & F_PLAY)
		return selected;
	free(selected->filename);
	free(selected->path);
	free(selected->title);
	free(selected->artist);
 	playlist->length--;
	if ((fnext = selected->next)) {
		if ((fprev = selected->prev)) {
			fprev->next = fnext;
			fnext->prev = fprev;
		} else {
			fnext->prev = NULL;
			playlist->head = playlist->top = fnext;
		}
	 	fnext->flags |= F_SELECTED;
		if (selected == playlist->top)
			playlist->top = fnext;
		playlist->selected = fnext;
	 	free(selected);
	 	return fnext;
	} else if ((fprev = selected->prev)) {
		fprev->next = NULL;
		playlist->tail = fprev;
		playlist->where--;
		fprev->flags |= F_SELECTED;
		if (playlist->top == selected) {
			for (i = 0, ftmp = fprev; ftmp && ftmp->prev && (i < maxy); ftmp = ftmp->prev, i++);
			playlist->top = ftmp;
		}
		free(selected);
		return fprev;
	} else {
		free(selected);
		playlist->head = playlist->top = playlist->tail = NULL;
		return NULL;
	}
}

wlist *
randomize_list (wlist *playlist)
{
	int i = playlist->length, j, k, selected = 0, top = 0;
	flist *ftmp = NULL, *newlist = NULL, **farray = NULL;
	
	if (i < 2)
		return playlist;
	if (!(farray = (flist **) calloc (i, sizeof(flist *))))
		return playlist;
	for (ftmp = playlist->head, j = 0; ftmp; ftmp = ftmp->next, j++) {
		if (ftmp == playlist->top)
			top = j;
		if (ftmp->flags & F_SELECTED) {
			selected = j;
			ftmp->flags &= ~F_SELECTED;
		}
		farray[j] = ftmp;
	}
	k = (int) ((float)i--*rand()/(RAND_MAX+1.0));
	newlist = farray[k];
	newlist->prev = NULL;
	farray[k] = NULL;
	playlist->head = playlist->top = newlist;
	for (ftmp = NULL; i; i--) {
		k = (int) ((float)i*rand()/(RAND_MAX+1.0));
		for (j = 0; j <= k; j++)
			if (farray[j] == NULL)
				k++;
		ftmp = farray[k];
		farray[k] = NULL;
		newlist->next = ftmp;
		if (ftmp) {
			ftmp->prev = newlist;
			newlist = ftmp;
		}
	}
	for (ftmp = playlist->head, i = 0; i < top; ftmp = ftmp->next, i++);
	playlist->top = ftmp;
	for (; i < selected; ftmp = ftmp->next, i++);
	ftmp->flags |= F_SELECTED;
	playlist->selected = ftmp;
	playlist->tail = newlist;
	newlist->next = NULL;
	free(farray);
	return playlist;
}

wlist *
add_to_playlist (wlist *playlist, flist *file)
{
	flist *ftmp = NULL, *newfile;
	int i = 0, maxx, maxy;

	getmaxyx(play->win, maxy, maxx);
	maxx = maxy - 2;
	maxy -= 3;

	/* either create a new playlist, or grab the tail */
	ftmp = playlist->tail;
	newfile = calloc(1, sizeof(flist));
	if (!ftmp)
		playlist->head = playlist->top = newfile;
	newfile->filename = strdup(file->filename);
	newfile->path = strdup(file->path);
	newfile->length = file->length;
	newfile->bitrate = file->bitrate;
	newfile->frequency = file->frequency;
	if (file->title)
		newfile->title = strdup(file->title);
	if (file->artist)
		newfile->artist = strdup(file->artist);
	if (ftmp) {
		ftmp->next = newfile;
		newfile->prev = ftmp;
	}
	playlist->tail = newfile;
	if (!playlist->selected) {
		playlist->selected = newfile;
		newfile->flags |= F_SELECTED;
		playlist->where = 1;
	}
	if (conf->p_advance) {
		playlist->selected->flags &= ~F_SELECTED; /* could be a wasted dupe */
		playlist->selected = newfile;
		playlist->where = ++playlist->length;
		newfile->flags |= F_SELECTED;
		if (playlist->length < maxx)
			return playlist;
		/* this is inefficient, we could "store" more data about the list, but
		 * who really cares. :) */
		for (ftmp = newfile; ftmp && i < maxy; i++, ftmp = ftmp->prev)
			if (ftmp == playlist->top)
				return playlist;
		playlist->top = ftmp;
	} else
		++playlist->length;
	return playlist;
}

int
do_read_playlist(Input *input)
{
	wmove(menubar->win, 0, 0);
	wbkgd(menubar->win, colors[MENU_BACK]);
	wclrtoeol(menubar->win);
	active = old_active;
	my_mvwaddstr(menubar->win, 0, 28, colors[MENU_TEXT], version_str);
	play->contents.list = read_playlist(play->contents.list, input->buf);
	if (play->contents.list->head) {
		info->contents.play = play->contents.list->selected;
		active->deactivate(active);
		active->flags &= ~W_ACTIVE;
		active->update(active);
		play->activate((active = play));
		active->flags |= W_ACTIVE;
		play->update(play);
		info->update(info);
	}
	free(input);
	menubar->inputline = NULL;
	curs_set(0);
	update_panels();
	doupdate();
	return 1;
}

int
do_save_playlist(Input *input)
{
	wmove(menubar->win, 0, 0);
	wbkgd(menubar->win, colors[MENU_BACK]);
	wclrtoeol(menubar->win);
	active = old_active;
	my_mvwaddstr(menubar->win, 0, 28, colors[MENU_TEXT], version_str);
	write_playlist(play->contents.list, input->buf);
	free(input);
	menubar->inputline = NULL;
	curs_set(0);
	update_panels();
	doupdate();
	return 1;
}

int
write_playlist (wlist *playlist, const char *file)
{
	FILE *fp;
	flist *ftmp;
	
	if (!playlist || !playlist->head)
		return 0;
	if (!(fp = fopen(file, "w")))
		return 0;
	
	for (ftmp = playlist->head; ftmp; ftmp = ftmp->next)
		fprintf(fp, "%s/%s:%s:%s:%ld:%d:%d\n", ftmp->path, ftmp->filename,
			ftmp->artist?:"", ftmp->title?:"", (long int)ftmp->length, ftmp->bitrate, ftmp->frequency);
	fclose(fp);
	return 1;
}
