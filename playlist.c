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
	else if (!ftmp->next)
		return;
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
	else if (ftmp->prev)
		ftmp = ftmp->prev;
	
	if (!jump_to_song(list, ftmp)) 
		stop_player(list);
}


int
jump_to_song(wlist *list, flist *next)
{
	char buf[BIG_BUFFER_SIZE+1];
	FILE *activefile;
	flist *ftmp;

	
	
	
//	next = next_valid(list, next, KEY_DOWN);
// it is in the playlist so it should be valid anyway and to check it slows things way down
	
	if (!next)
		return 0;
	
		
	list->playing = next;
	memset(buf, 0, sizeof(buf));
	snprintf(buf, BIG_BUFFER_SIZE, "%s", list->playing->fullpath);
	send_cmd(LOAD, buf);

	clear_play_info();
	p_status = PLAYING;
	
	play->update(play);
	info->update(info);
	update_title(playback);
	doupdate();

	for (ftmp = list->head, list->whereplaying=0; ftmp!=list->playing; ftmp=ftmp->next)
		list->whereplaying++;

	activefile = fopen(conf->statefile,"w");
	if (activefile) {
		fprintf(activefile,"         Now playing:  %s  (by)  %s  (from)  %s    \n", list->playing->filename, 
		(list->playing->artist ? list->playing->artist : "Unknown"), (list->playing->album ? list->playing->album : "Unknown"));
		fclose(activefile);
	}
	return 1;
}

void
stop_player(wlist *list)
{
	FILE *activefile;
	

	
	if (list->playing) {
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
randomize_list(wlist *list)
{
	int i = list->length, j, k;
	flist *ftmp = NULL, *newlist = NULL, **farray = NULL;



	
	if (i < 2)
		return list;
	if (!(farray = (flist **) calloc(i, sizeof(flist *))))
		return list;
	for (ftmp = list->head, j = 0; ftmp; ftmp = ftmp->next, j++) 
		farray[j] = ftmp;
	k = (int)((float)i--*rand()/(RAND_MAX+1.0));
	newlist = farray[k];
	newlist->prev = NULL;
	farray[k] = NULL;
	list->head = list->top = newlist;
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
	list->selected = list->head;
	list->where = 1;
	list->wheretop = 0;
	list->tail = newlist;
	newlist->next = NULL;
	free(farray);
	return list;
}


void
add_to_playlist_recursive(wlist *list, flist *position, flist *file)
{
	char *prevpwd = NULL;
	wlist *templist = NULL;

	if (!(file->flags & F_DIR))
		return;

	templist = calloc(1, sizeof(wlist));
	prevpwd = getcwd(NULL, 0);

	read_mp3_list(templist, file->fullpath, L_NEW);
	if (!strncmp(templist->selected->filename, "../", 3))
		templist->selected = templist->head->next; // skip ../ entry
	
	while (templist->selected) {
		if (templist->selected->flags & F_DIR)
			add_to_playlist_recursive(list, list->tail, templist->selected);
		else if (!(templist->selected->flags & F_PLAYLIST))
			add_to_playlist(list, list->tail, templist->selected);
		
		templist->selected = next_valid(templist, templist->selected->next, KEY_DOWN);
	}

	wlist_clear(templist);
	free(templist);
	chdir(prevpwd);
	free(prevpwd);
}




void
add_to_playlist(wlist *list, flist *position, flist *file)
{
	flist *newfile;
	
	newfile = calloc(1, sizeof(flist));
	
	/* remove tracknumber if it exists and user wants it*/
	if (!(conf->c_flags & C_TRACK_NUMBERS)){
		char *p = strip_track_numbers(file->filename);
		newfile->filename = strdup(p);
	} else
		newfile->filename = strdup(file->filename);
	
	if (strlen(newfile->filename) == 0) {
		free(newfile->filename);
		newfile->filename = strdup("...");
	}
		
	newfile->path = strdup(file->path);
	
	newfile->fullpath = strdup(file->fullpath);
	
	if (file->genre)
		newfile->genre = strdup(file->genre);

	if (file->album)
		newfile->album = strdup(file->album);
	
	if (file->artist)
		newfile->artist = strdup(file->artist);

	
	wlist_add(list, position, newfile); 

	if (conf->c_flags & C_PADVANCE) {
		list->selected = newfile;
		list->where = list->length;
	} 
	return;
}


