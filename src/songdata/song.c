#include "songdata.h"

// these are NOT for external use, use songdata_clear instead
static void	free_list ( songdata_song * );
static void	free_songdata_song ( songdata_song * );


void songdata_add ( songdata *list, songdata_song *position, songdata_song *new )
{
  songdata_song *after;
  if ( !list )
    abort();

	// if position == NULL add to front
  if ( position == NULL )
  {
    list->head = new;
    list->tail = new;
    new->prev = new->next = NULL;
  }
  else 	if ( position == list->tail )
  {
    new->next = NULL;
    new->prev = position;
    position->next = new;
    list->tail = new;
  }
  else
  {
    after = position->next;
    new->next = after;
    new->prev = position;
    after->prev = new;
    position->next = new;
  }

  if ( list->selected == NULL )   //list was empty
  {
    list->head = list->tail = list->selected = new;
    list->where = 1;
  }
  list->length++;
}

void
    songdata_del ( songdata *list, songdata_song *position )
{
  songdata_song *before = position->prev, *after = position->next;
  if ( !list->head )
    return; // list is empty;

  list->length--;

	//fixup deleted selected
  if ( position == list->selected )
  {
    if ( after )
    {
      list->selected = after;
    }
    else
    {
      list->selected = before;
      if ( before )
        list->where--;
    }
  }

	//fixup our linked list
  if ( after )
    after->prev = before;
  else
    list->tail = before;
  if ( before )
    before->next = after;
  else
    list->head = after;



  free_songdata_song ( position );
}

void
    songdata_clear ( songdata *list )
{
  if ( list->head )
    free_list ( list->head );
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

static void
    free_list ( songdata_song *list )
{
  songdata_song *ftmp, *next;

  for ( ftmp = list; ftmp; )
  {
    next = ftmp->next;
    if ( ftmp )
    {
      free_songdata_song ( ftmp );
      free ( ftmp );
    }
    ftmp = next;
  }
	//free(ftmp);
}

static void
    free_songdata_song ( songdata_song *file )
{
  if ( !file )
    return;
  if ( file->filename )
    free ( file->filename );
  if ( file->artist )
    free ( file->artist );
  if ( file->path )
    free ( file->path );
  if ( file->relpath )
    free ( file->relpath );
  if ( file->fullpath )
    free ( file->fullpath );
  if ( file->album )
    free ( file->album );
  if ( file->genre )
    free ( file->genre );
  if ( file->title )
    free ( file->title );
}