#include "top.h"
#include "defs.h"
#include "struct.h"
#include "list.h"

// these are NOT for external use, use wlist_clear instead
static void	free_list(flist *);
static void	free_flist(flist *);
static dirstack *dirstack_top = NULL;



void
wlist_add(wlist *list, flist *position, flist *new) 
{ /* Add entry to the list at position "position" */
	flist *after;
	
	
	
	if (!list)
		abort();

	if (position == NULL) {
		/* Position == NULL, add entry to list->head */
		list->head = new;
		list->tail = new;
		new->prev = new->next = NULL;
	} else 	if (position == list->tail) {
		/* Position == tail, add entry to list->tail */
		new->next = NULL;
		new->prev = position;
		position->next = new;
		list->tail = new;
	} else {
		/* Add new entry after Position in list */
		after = position->next;
		new->next = after;
		new->prev = position;
		after->prev = new;
		position->next = new;
	}

	if (list->selected == NULL){ 
		/* List was empty, construct new head and tail */
		list->head = list->tail = list->selected = new;
		list->where = 1;
	}
	list->length++;
}

void 
wlist_del(wlist *list, flist *position) 
{ /* Delete listentry at position */
	
	flist *before = position->prev, *after = position->next;
	
	
	
	if (!list->head)
		return; // list is empty;
	
	list->length--;

	if (position == list->selected) {
		/* Fixup deleted selected */
		if (after) {
			list->selected = after;
		} else { 
			list->selected = before;
			if (before)
				list->where--;
		}
	}

	/* Fixup our linked list */
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
wlist_move(wlist *new, wlist *old) 
{ /* move data of old in to structure new, clear old */
	
	wlist_clear(new);
	new->from = old->from;
	new->head = old->head;
	new->tail = old->tail;
	new->top = old->top;
	new->bottom = old->bottom;
	new->selected = old->selected;
	new->lastadded = old->lastadded;
	new->playing = old->playing;
	new->length = old->length;
	new->where = old->where;
	new->wheretop = old->wheretop;
	new->whereplaying = old->whereplaying;
	new->flags = old->flags;
	old->length = 
	old->where = 
	old->wheretop =
	old->flags = 0;

	old->head = 
	old->tail = 
	old->top = 
	old->bottom = 
	old->selected = 
	old->playing = NULL;

}	

void
move_backward(wlist *list)
{ /* Move selected entry one place backward in the list */
	
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
{ /* Move selected entry one place forward in the list */
	
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
{ /* Clear list "list", freeing up the memory it might be using
     and initialise list to a known state before use */
	
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
{ /* Recursively free the linked list "list" */
	
	flist *ftmp, *next;
	
	for (ftmp = list; ftmp;) {
		next = ftmp->next;
		if (ftmp) {
			free_flist(ftmp);
			free(ftmp);
		}
		ftmp = next;
	}
}

void
free_flist(flist *file)
{ /* Free memory of 1 flist structure */
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
}

void
dirstack_push (const char *fullpath, const char *filename, wlist *list)
{ /* Put directory entry on the stack, with a copy of its wlist */
	dirstack *tmp = calloc (1, sizeof(dirstack));
	wlist *cachelist = (wlist *) calloc (1, sizeof (wlist));



	tmp->fullpath = strdup(fullpath);
	tmp->filename = strdup(filename);
	wlist_move(cachelist, list);
	tmp->list = cachelist;
	tmp->prev = dirstack_top;
	dirstack_top = tmp;
}

char *
dirstack_fullpath (void)
{ /* Get the fullpath of the directory at the top of the stack */
	if (dirstack_top) 
		return dirstack_top->fullpath;
	else
		abort();
}

char *
dirstack_filename (void)
{ /* Get the filename of the directory at the top of the stack */
	if (dirstack_top) 
		return dirstack_top->filename;
	else
		abort();
}

wlist *
dirstack_listcache (void)
{ /* Get the list of the directory at the top of the stack */
	if (dirstack_top) 
		return dirstack_top->list;
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
{ /* remove topmost directory from the directory stack */
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
