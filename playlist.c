#include "top.h"
#include "defs.h"
#include "struct.h"
#include "playlist.h"
#include "files.h"
#include "mpgcontrol.h"
#include "misc.h"
#include "list.h"
#include "window.h"
#include "mjs.h"
#include "extern.h"

void
play_next_song(wlist *list)
{
	flist *ftmp = list->playing;

	if (!ftmp)
		return;

	if ((conf->c_flags & C_LOOP) && !ftmp->next)
		ftmp = list->head;
	else
		ftmp = ftmp->next;
	if (!jump_to_song(list, ftmp)) 
		stop_player(list);
}

void
play_prev_song(wlist *list)
{
	flist *ftmp = list->playing;

	if (!ftmp)
		return;
	if ((conf->c_flags & C_LOOP) && !ftmp->prev)
		ftmp = list->tail;
	else
		ftmp = ftmp->prev;
	if (!jump_to_song(list, ftmp)) 
		stop_player(list);
}


int
jump_to_song(wlist *list, flist *next)
{
	char buf[BIG_BUFFER_SIZE+1];
	FILE *logfile;
	FILE *activefile;
	time_t timevalue;

	next = next_valid(list, next, KEY_DOWN);
	
	if (!next)
		return 0;
	
	list->playing = next;
	info->contents.play = next;
	memset(buf, 0, sizeof(buf));
	snprintf(buf, BIG_BUFFER_SIZE, "%s", list->playing->fullpath);
	send_cmd(LOAD, buf);

	clear_play_info();
	p_status = PLAYING;
			
	timevalue = time(NULL);
	activefile = fopen(conf->statefile,"w");
	if (activefile) {
		fprintf(activefile,"         Now playing:  %s  (by)  %s  (from)  %s    \n", list->playing->filename, list->playing->artist, list->playing->album);
		fclose(activefile);
	}
	logfile = fopen(conf->logfile,"a");
	if (logfile) {
		fprintf(logfile,"%.24s\t%s\n",ctime(&timevalue), list->playing->fullpath);
		fclose(logfile);
	}
	
	play->update(play);
	update_title(playback);
	doupdate();
	return 1;
}

void
stop_player(wlist *list)
{
	FILE *activefile;
		
	if (list->playing) {
		info->contents.play = NULL;
		list->playing->flags &= ~F_PAUSED;
		list->playing = NULL;
		play->update(play);
	}

	if (p_status == PAUSED)
		send_cmd(PAUSE);

	p_status = STOPPED;
	send_cmd(STOP);

	update_title(playback);
	clear_play_info();
	
	activefile = fopen(conf->statefile,"w");
	if (activefile) {
		fprintf(activefile,"%s","                      \n");
		fclose(activefile);
	}

	return;
}

void
pause_player(wlist *list)
{
	FILE *activefile;
	list->playing->flags |= F_PAUSED;
	play->update(play);
	p_status = PAUSED;
	send_cmd(PAUSE);
	activefile = fopen(conf->statefile,"w");
	if (activefile) {
		fprintf(activefile,"         Now playing:  %s  (by)  %s  (from)  %s    \n", list->playing->filename, list->playing->artist, list->playing->album);
		fclose(activefile);
	}

	return;
}

void
resume_player(wlist *list)
{
	FILE *active;
	list->playing->flags &= ~F_PAUSED;
	play->update(play);
	p_status = PLAYING;
	send_cmd(PAUSE);
	active = fopen(conf->statefile,"w");
	if (active) {
		fprintf(active,"%s","         * Pause *    \n");
		fclose(active);
	}
	return;
}

wlist *
randomize_list(wlist *playlist)
{
	int i = playlist->length, j, k;
	flist *ftmp = NULL, *newlist = NULL, **farray = NULL;
	
	if (i < 2)
		return playlist;
	if (!(farray = (flist **) calloc(i, sizeof(flist *))))
		return playlist;
	for (ftmp = playlist->head, j = 0; ftmp; ftmp = ftmp->next, j++) 
		farray[j] = ftmp;
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
	playlist->selected = playlist->head;
	playlist->where = playlist->wheretop = 0;
	playlist->tail = newlist;
	newlist->next = NULL;
	free(farray);
	return playlist;
}


void
add_to_playlist_recursive(wlist *playlist, flist *position, flist *file)
{
	char *prevpwd = NULL;
	wlist *templist = NULL;
	if (!(file->flags & F_DIR))
		return;

	templist = calloc(1, sizeof(wlist));
	prevpwd = getcwd(NULL, 0);
	chdir(file->fullpath);

	read_mp3_list(templist);
	if (templist->head)
		sort_songs(templist);
	templist->selected = templist->head->next; // skip ../ entry
	
	while (templist->selected) {
		if (templist->selected->flags & F_DIR)
			add_to_playlist_recursive(playlist, playlist->tail, templist->selected);
		else if (!(templist->selected->flags & F_PLAYLIST))
			add_to_playlist(playlist, playlist->tail, templist->selected);
		
		templist->selected = next_valid(templist, templist->selected->next, KEY_DOWN);
	}

	wlist_clear(templist);
	free(templist);
	chdir(prevpwd);
	free(prevpwd);
}

void
add_to_playlist(wlist *playlist, flist *position, flist *file)
{
	flist *newfile;
	
	newfile = calloc(1, sizeof(flist));
	
	/* remove tracknumber if it exists and user wants it*/
	if (!(conf->c_flags & C_TRACK_NUMBERS)) {
		if ((file->filename[0]>='0') & (file->filename[0]<='9')) {
			if ((file->filename[1]>='0') & (file->filename[1]<='9'))
				newfile->filename = strdup(file->filename+3);	
			else
				newfile->filename = strdup(file->filename+2);
		} else if (!strncasecmp(file->filename,"cd",2))
			newfile->filename = strdup(file->filename+7);	
	}
	if (!newfile->filename)
		newfile->filename = strdup(file->filename);
	if (strlen(newfile->filename) == 0) {
		free(newfile->filename);
		newfile->filename = strdup("...");
	}
		
	newfile->path = strdup(file->path);
	
	newfile->fullpath = strdup(file->fullpath);
	
	if (file->album)
		newfile->album = strdup(file->album);
	
	if (file->artist)
		newfile->artist = strdup(file->artist);

	
	wlist_add(playlist, position, newfile); 

	if (conf->c_flags & C_PADVANCE) {
		playlist->selected = newfile;
		playlist->where = playlist->length;
	} 
	return;
}


