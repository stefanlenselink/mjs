
#include "list.h"

#include <string.h>
#include <stdlib.h>

// these are NOT for external use, use wlist_clear instead
static void	free_list(flist *);
static void	free_flist(flist *);
static dirstack *dirstack_top = NULL;


void
wlist_add(wlist *list, flist *position, flist *new) 
{
	flist *after;
	if (!list)
		abort();

	// if position == NULL add to front
	if (position == NULL) {
		list->head = new;
		list->tail = new;
		new->prev = new->next = NULL;
	} else 	if (position == list->tail) {
		new->next = NULL;
		new->prev = position;
		position->next = new;
		list->tail = new;
	} else {
		after = position->next;
		new->next = after;
		new->prev = position;
		after->prev = new;
		position->next = new;
	}

	if (list->selected == NULL){ //list was empty
		list->head = list->tail = list->selected = new;
		list->where = 1;
	}
	list->length++;
}

void 
wlist_del(wlist *list, flist *position) 
{
	flist *before = position->prev, *after = position->next;
	if (!list->head)
		return; // list is empty;
	
	list->length--;

	//fixup deleted selected
	if (position == list->selected) {
		if (after) {
			list->selected = after;
		} else { 
			list->selected = before;
			if (before)
				list->where--;
		}
	}

	//fixup our linked list
	if (after) 
		after->prev = before;
	else
		list->tail = before;
	if (before)
		before->next = after;
	else
		list->head = after;


	
	free_flist(position);
}

void
move_backward(wlist *list)
{
	flist *f1,*f2,*f3,*f4;

	f3 = list->selected;
	f2 = f3->prev;
	f1 = f2->prev;
	f4 = f3->next;

	f3->prev = f1;
	if (f1) 
		f1->next = f3;
	else {
		list->head = f3;
		list->top = f3;
	}
	f3->next = f2;
	f2->prev = f3;
	f2->next = f4;
	if (f4)
		f4->prev = f2;
	else 
		list->tail = f2;
	list->where--;
	return;
}

void
move_forward(wlist *list)
{
	flist *f1,*f2,*f3,*f4;

	f2 = list->selected;
	f3 = f2->next;
	f1 = f2->prev;
	f4 = f3->next;

	if (f1)
		f1->next = f3;
	else {
		list->head = f3;
		list->top = f3;
	}
	f3->prev = f1;
	f3->next = f2;
	f2->prev = f3;
	f2->next = f4;
	if (f4)
		f4->prev = f2;
	else	
		list->tail = f2;
	list->where++;
	return;
}
	
void
wlist_clear(wlist *list) 
{
	if (list->head)
		free_list(list->head);
	list->length = 
	list->where = 
	list->wheretop =
	list->flags = 0;

	list->head = 
	list->tail = 
	list->top = 
	list->bottom = 
	list->selected = 
	list->playing = NULL;
}
	
void
free_list(flist *list)
{
	flist *ftmp, *next;
	
	for (ftmp = list; ftmp;) {
		next = ftmp->next;
		if (ftmp) {
			free_flist(ftmp);
			free(ftmp);
		}
		ftmp = next;
	}
	//free(ftmp);
}

void
free_flist(flist *file)
{
	if (!file)
		return;
	if (file->filename)
		free(file->filename);
	if (file->artist)
		free(file->artist);
	if (file->path)
		free(file->path);
	if (file->relpath)
		free(file->relpath);
	if (file->fullpath)
		free(file->fullpath);
	if (file->album)
		free(file->album);
	if (file->genre)
		free(file->genre);
	if (file->title)
		free(file->title);
}

void
dirstack_push (const char *fullpath, const char *filename)
{
	dirstack *tmp = calloc (1, sizeof(dirstack));
	tmp->fullpath = strdup(fullpath);
	tmp->filename = strdup(filename);
	tmp->prev = dirstack_top;
	dirstack_top = tmp;
}

char *
dirstack_fullpath (void)
{
	if (dirstack_top) 
		return dirstack_top->fullpath;
	else
		abort();
}

char *
dirstack_filename (void)
{
	if (dirstack_top) 
		return dirstack_top->filename;
	else
		abort();
}

int
dirstack_empty (void)
{
	if (dirstack_top)
		return 0;
	else
		return 1;
}

void
dirstack_pop (void)
{
	dirstack *tmp;
	if (dirstack_top) {
		tmp = dirstack_top;
		free(tmp->fullpath);
		free(tmp->filename);
		dirstack_top = tmp->prev;
		free(tmp);
	} else
		abort();
}
