#include "top.h"
#include "defs.h"
#include "struct.h"
#include "playlist.h"
#include "files.h"
#include "mpgcontrol.h"
#include "misc.h"
#include "window.h"
#include "mms.h"
#include "extern.h"

static flist	*parse_playlist_line(flist *, char *);
static void	 free_playlist(wlist *);

void
play_next_song(void)
{
	flist *ftmp = play->contents.list->playing;

	if (!ftmp)
		return;

	ftmp->flags &= ~F_PLAY;
	if (ftmp->flags & F_SELECTED)
		ftmp->colors = colors[SELECTED];
	else
		ftmp->colors = colors[UNSELECTED];
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
	if (ftmp->flags & F_SELECTED)
		ftmp->colors = colors[SELECTED];
	else
		ftmp->colors = colors[UNSELECTED];
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
	flist *ftmp = play->contents.list->playing;

	if (!ftmp)
		return playlist;
	ftmp->flags &= ~F_PLAY;
	if (ftmp->flags & F_SELECTED)
		ftmp->colors = colors[SELECTED];
	else
		ftmp->colors = colors[UNSELECTED];
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
	return playlist;
}

int
jump_to_song(flist *selected)
{
	char buf[BIG_BUFFER_SIZE+1];
	wlist *playlist = play->contents.list;

	selected = next_valid(play, selected, KEY_DOWN);
	
	if (!playlist || !selected)
		return 0;
	
	if (info->contents.play) {
		info->contents.play->flags &= ~F_PLAY;
		if (info->contents.play->flags & F_SELECTED)
			info->contents.play->colors = colors[SELECTED];
		else
			info->contents.play->colors = colors[UNSELECTED];
	}

	memset(buf, 0, sizeof(buf));
	snprintf(buf, BIG_BUFFER_SIZE, "%s/%s", selected->path, selected->filename);
	send_cmd(LOAD, buf);
	clear_play_info();
	p_status = 1;
	selected->flags |= F_PLAY;
	if (selected->flags & F_SELECTED)
		selected->colors = colors[SEL_PLAYING];
	else
		selected->colors = colors[PLAYING];
	playlist->playing = selected;
	info->contents.play = selected;
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
	flist *ftmp = play->contents.list->playing;
	
	if (ftmp) {
		info->contents.play = NULL;
		play->contents.list->playing = NULL;
		ftmp->flags &= ~F_PLAY;
		if (ftmp->flags & F_SELECTED)
			ftmp->colors = colors[SELECTED];
		else
			ftmp->colors = colors[UNSELECTED];
		play->update(play);
	}
	send_cmd(STOP);
	update_title(playback);
	return playlist;
}

wlist *
pause_player(wlist *playlist)
{
	extern int p_status;
	if (p_status == 1) {
		playlist->playing->flags |= F_PAUSED;
		p_status = 2;
	}
	else if (p_status == 2) {
		playlist->playing->flags &= ~F_PAUSED;
		p_status = 1;
	}
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
			if (ftmp->flags & F_PLAY)
				ftmp->colors = colors[PLAYING];
			else
				ftmp->colors = colors[UNSELECTED];
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
	if (ftmp->flags & F_PLAY)
		ftmp->colors = colors[SEL_PLAYING];
	else
		ftmp->colors = colors[SELECTED];
	playlist->selected = ftmp;
	playlist->tail = newlist;
	newlist->next = NULL;
	free(farray);
	return playlist;
}

wlist *
add_to_playlist(wlist *playlist, flist *file)
{
	flist *ftmp = NULL, *newfile;
	int i = 0, maxx, maxy;

	getmaxyx(play->win, maxy, maxx);
	maxx = maxy - 2;
	maxy -= 3;

	/* either create a new playlist, or grab the tail */
	ftmp = playlist->tail;
	newfile = calloc(1, sizeof(flist));
	newfile->colors = colors[UNSELECTED];
	if (!ftmp)
		playlist->head = playlist->top = newfile;
	newfile->filename = strdup(file->filename);
	newfile->path = strdup(file->path);
	newfile->fullpath = strdup(file->fullpath);
	newfile->length = file->length;
	newfile->bitrate = file->bitrate;
	newfile->frequency = file->frequency;
	newfile->genre = file->genre;
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
		newfile->colors = colors[SELECTED];
		playlist->where = 1;
	}
	if (conf->c_flags & C_PADVANCE) {
		playlist->selected->flags &= ~F_SELECTED; /* could be a wasted dupe */
		playlist->selected = newfile;
		playlist->where = ++playlist->length;
		newfile->flags |= F_SELECTED;
		newfile->colors = colors[SELECTED];
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
		active->update(active);
		play->activate((active = play));
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
write_playlist(wlist *playlist, const char *file)
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

wlist *
read_playlist(wlist *plist, const char *file)
{
	FILE *fp;
	flist *ftmp = NULL, *first = NULL, *last = NULL;
	wlist *playlist = plist;
	char buf[1025];
	struct stat sb;

	if (!plist)
		return plist;

	if (!(fp = fopen(file, "r")))
		return plist;
	plist = (wlist *)calloc(1, sizeof(wlist));

	if (playlist->head && pid)
		stop_player(playlist);

	first = ftmp = (flist *)calloc(1, sizeof(flist));

	while (!feof(fp)) {
		if (!fgets(buf, sizeof(buf), fp))
			break;
		if (!ftmp) {
			ftmp = (flist *)calloc(1, sizeof(flist));
			ftmp->colors = colors[UNSELECTED];
		}
		if (!(parse_playlist_line(ftmp, buf)) || (stat(ftmp->fullpath, &sb) != 0)) {
			free_flist(ftmp);
			memset(ftmp, 0, sizeof(flist));
			continue;
		}
		ftmp->prev = last;
		if (last)
			last->next = ftmp;
		last = ftmp;
		plist->length++;
		ftmp = NULL;
	}
	fclose(fp);
	plist->tail = last;
	if (first->filename) {
		plist->head = plist->selected = plist->top = first;
		first->flags |= F_SELECTED;
		first->colors = colors[SELECTED];
	} else
		free(first);
	if (plist->length > 0) {
		free_playlist(playlist);
		playlist = plist;
		playlist->where = 1;
	} else
		free(plist);
	return playlist;
}

/* 
 * parse the line. If we encounter an error, give up and tell the caller its
 * their problem. PS, im not a big fan of sscanf() :)
 */

static flist *
parse_playlist_line(flist *file, char *line)
{
	char *s, *p = line;

	if (!(s = strchr(line, ':')))
		return NULL;
	*s++ = '\0';
	file->fullpath = strdup(p);
	if (!(p = strrchr(p, '/')))
		return NULL;
	*p++ = '\0';
	file->path = strdup(line);
	file->filename = strdup(p);
	if (!(p = strchr(s, ':')))
		return NULL;
	*p++ = '\0';
	if (s && *s)
		file->artist = strdup(s);
	if (!(s = strchr(p, ':')))
		return NULL;
	*s++ = '\0';
	if (p && *p)
		file->title = strdup(p);
	if (!(p = strchr(s, ':')))
		return NULL;
	file->length = (time_t) strtoul(s, &p, 10);
	if (*p != ':')
		return NULL;
	file->bitrate = (short) strtoul(++p, &s, 10);
	if (*s != ':')
		return NULL;
	file->frequency = (int) strtoul(++s, &p, 10);
	return file;
}

static void
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
