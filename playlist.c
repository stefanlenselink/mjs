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

	if (!jump_to_song(ftmp->next)) {
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		ftmp->flags &= ~F_PLAY;
		play->update(play);
	}
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

wlist *
jump_forward (wlist *playlist)
{
	flist *ftmp = info->contents.play;

	if (!ftmp)
		return playlist;
	if (!jump_to_song(ftmp->next)) {
		stop_player(playlist);
		ftmp->flags &= ~F_PLAY;
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
	if (!jump_to_song(ftmp->prev)) {
		stop_player(playlist);
		ftmp->flags &= ~F_PLAY;
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		show_list(play);
	}
	return playlist;
}

flist *
delete_selected (wlist *playlist)
{
	flist *fnext, *fprev, *selected, *ftmp;
	int i, maxx, maxy;
	if (!playlist)
		return NULL;
	selected = playlist->selected;
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
	else
		playlist->selected->flags &= ~F_SELECTED;
	playlist->selected = newfile;
	newfile->filename = strdup(file->filename);
	newfile->path = strdup(file->path);
	newfile->length = file->length;
	newfile->bitrate = file->bitrate;
	newfile->frequency = file->frequency;
	if (file->title)
		newfile->title = strdup(file->title);
	if (file->artist)
		newfile->artist = strdup(file->artist);
	if (ftmp)
		ftmp->next = newfile;
	playlist->where = ++playlist->length;
	newfile->flags |= F_SELECTED;
	newfile->prev = ftmp;
	newfile->next = NULL;
	newfile->flags &= ~F_PLAY;
	playlist->tail = newfile;
	if (playlist->length < maxx)
		return playlist;
	/* this is inefficient, we could "store" more data about the list, but
	 * who really cares. :) */
	for (ftmp = newfile; ftmp && i < maxy; i++, ftmp = ftmp->prev)
		if (ftmp == playlist->top)
			return playlist;
	playlist->top = ftmp;
	return playlist;
}
