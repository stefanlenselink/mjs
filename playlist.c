#include "top.h"
#include "defs.h"
#include "struct.h"
#include "playlist.h"
#include "files.h"
#include "mpgcontrol.h"
#include "misc.h"
#include "window.h"
#include "mjs.h"
#include "extern.h"

void
calculate_duration(flist *list)
{
        flist *ftmp;
        play->contents.list->duration=0; 
	for (ftmp = list; ftmp; ftmp = ftmp->next) {
	play->contents.list->duration+=(ftmp->length);
        }
}
void
play_next_song(void)
{
	flist *ftmp = play->contents.list->playing;

	if (!ftmp)
		return;

	ftmp->flags &= ~F_PLAY;
	if ((conf->c_flags & C_LOOP) && !ftmp->next)
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
jump_forward(wlist *playlist)
{
	flist *ftmp = play->contents.list->playing;

	if (!ftmp)
		return playlist;
	ftmp->flags &= ~F_PLAY;
	if ((conf->c_flags & C_LOOP) && !ftmp->next)
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
jump_backward(wlist *playlist)
{
	flist *ftmp = playlist->playing;
	int duration = playlist->duration; 
	int length = playlist->playing->length;

	if (!ftmp)
		return playlist;
	ftmp->flags &= ~F_PLAY;
	if ((conf->c_flags & C_LOOP) && !ftmp->prev)
		ftmp = play->contents.list->tail;
	else
		ftmp = ftmp->prev;
	if (!jump_to_song(ftmp)) {
		stop_player(playlist);
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		show_list(play);
	}
	playlist->duration=((length+duration));
	return playlist;
}

wlist *
move_backward(wlist *playlist)
{
	flist *f1,*f2,*f3,*f4;

	f3 = playlist->selected;
	f2 = f3->prev;
	f1 = f2->prev;
	f4 = f3->next;

	f3->prev = f1;
	if (f1) 
		f1->next = f3;
	else {
		playlist->head = f3;
		playlist->top = f3;
	}
	f3->next = f2;
	f2->prev = f3;
	f2->next = f4;
	if (f4)
		f4->prev = f2;
	else 
		playlist->tail = f2;
	playlist->where--;
	return playlist;
}

wlist *
move_forward(wlist *playlist)
{
	flist *f1,*f2,*f3,*f4;

	f2 = playlist->selected;
	f3 = f2->next;
	f1 = f2->prev;
	f4 = f3->next;

	if (f1)
		f1->next = f3;
	else {
		playlist->head = f3;
		playlist->top = f3;
	}
	f3->prev = f1;
	f3->next = f2;
	f2->prev = f3;
	f2->next = f4;
	if (f4)
		f4->prev = f2;
	else	
		playlist->tail = f2;
	playlist->where++;
	return playlist;
}

int
jump_to_song(flist *selected)
{
	extern int p_status;
	char buf[BIG_BUFFER_SIZE+1];
	wlist *playlist = play->contents.list;

	selected = next_valid(play, selected, KEY_DOWN);
	
	if (!playlist || !selected)
		return 0;
	
	if (info->contents.play) 
		info->contents.play->flags &= ~F_PLAY;
		
	memset(buf, 0, sizeof(buf));
	snprintf(buf, BIG_BUFFER_SIZE, "%s", selected->fullpath);
	send_cmd(LOAD, buf);
	clear_play_info();
	p_status = PLAYING;
	selected->flags |= F_PLAY;
	playlist->playing = selected;
	info->contents.play = selected;
	play->contents.list->duration -= selected->length;
	if (active == info)
		info->update(info);
	play->update(play);
	update_title(playback);
	doupdate();
	return 1;
}

wlist *
stop_player(wlist *playlist)
{
	extern int p_status;
	flist *ftmp = play->contents.list->playing;
	
	if (ftmp) {
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		ftmp->flags &= ~F_PLAY;
		ftmp->flags &= ~F_PAUSED;
		play->update(play);
	}
	if (p_status == PAUSED)
		send_cmd(PAUSE);
	p_status = STOPPED;
	send_cmd(STOP);
	update_title(playback);
	doupdate();
	return playlist;
}

wlist *
pause_player(wlist *playlist)
{
	extern int p_status;
	playlist->playing->flags |= F_PAUSED;
	p_status = PAUSED;
	send_cmd(PAUSE);
	return playlist;
}

wlist *
resume_player(wlist *playlist)
{
	extern int p_status;
	playlist->playing->flags &= ~F_PAUSED;
	p_status = PLAYING;
	send_cmd(PAUSE);
	return playlist;
}

wlist *
randomize_list(wlist *playlist)
{
	int i = playlist->length, j, k, selected = 0, top = 0;
	flist *ftmp = NULL, *newlist = NULL, **farray = NULL;
	
	if (i < 2)
		return playlist;
	if (!(farray = (flist **) calloc(i, sizeof(flist *))))
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
	k = (int)((float)i--*rand()/(RAND_MAX+1.0));
	newlist = farray[k];
	newlist->prev = NULL;
	farray[k] = NULL;
	playlist->head = playlist->top = newlist;
	for (ftmp = NULL; i; i--) {
		k = (int)((float)i*rand()/(RAND_MAX+1.0));
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

void
add_to_playlist(wlist *playlist, flist *position, flist *file)
{
	flist *newfile, *head = NULL, *tail = NULL;
	int i = 0, maxx, maxy;

	getmaxyx(play->win, maxy, maxx);
	maxx = maxy - 2;
	maxy -= 3;
	
	if (position) {
		head = position;
		tail = position->next;
	}
	
	/* either create a new playlist, or grab the tail */
	newfile = calloc(1, sizeof(flist));
	if (!head)
		playlist->head = playlist->top = newfile;
	/* remove tracknumber if it exists */
	if (((file->filename[0]=='0')|(file->filename[0]=='1')) & (!(conf->c_flags & C_TRACK_NUMBERS)))
		newfile->filename = strdup(file->filename+3);	
	else
		newfile->filename = strdup(file->filename);
		
	newfile->path = strdup(file->path);
	newfile->fullpath = strdup(file->fullpath);
	newfile->length = file->length;
	if (file->album)
		newfile->album = strdup(file->album);
	if (file->artist)
		newfile->artist = strdup(file->artist);
	if (head) {
		head->next = newfile;
		newfile->prev = head;
	}
	if (tail) {
		newfile->next = tail;
		tail->prev = newfile;
	}
	else 
		playlist->tail = newfile;
	playlist->duration += file->length;
	if (!playlist->selected) {
		playlist->selected = newfile;
		newfile->flags |= F_SELECTED;
		playlist->where = 1;
	}
	if (conf->c_flags & C_PADVANCE) {
		playlist->selected->flags &= ~F_SELECTED; /* could be a wasted dupe */
		playlist->selected = newfile;
		playlist->where = ++playlist->length;
		newfile->flags |= F_SELECTED;
		if (playlist->length < maxx)
			return;
		/* this is inefficient, we could "store" more data about the list, but
		 * who really cares. :) */
		for (head = newfile; head && i < maxy; i++, head = head->prev)
			if (head == playlist->top)
				return;
		playlist->top = head;
	} else
		++playlist->length;
	return;
}

void
free_playlist(wlist *playlist)
{
	flist *ftmp;
	
	if (!playlist)
		return;
	for (ftmp = playlist->head; ftmp; ftmp = ftmp->next) {
		free_flist(ftmp);
		free(ftmp->prev);
	}
	free(ftmp);
	memset(playlist, 0, sizeof(playlist));
}
