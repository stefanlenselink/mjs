#include "defs.h"
#include "songdata.h"
#include "disk_songdata.h"
#include "config/config.h"
#include "dirstack.h"

#include "log.h"

#include "gui/gui.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ncurses.h>
#include <libgen.h>


/* 	path -> path including trailing slash
	filename -> filename without any slashes
	count -> pointer to int

	mp3path = /pub/mp3/
    
	With genre:
    |*************************fullpath*****************************|
    /pub/mp3/Pop/Cranberries/Bury the hatchet/01 Animal instinct.mp3
             |*************path*************| |*****filename***|
             |*| |*Artist**| |*****Album****|
             |
             \-Genre

	|**************abspath******************|

	
	Without genre
    |***************fullpath*************|
    /pub/mp3/Cranberries/Bury the hatchet/01 Animal instinct.mp3
             |************path**********| |***filename***|
    |******abspath**********************|
*/

extern Config * conf;
/* colors */
static int * colors;

songdata *mp3list;

static void	read_mp3_list_file ( songdata *, const char *, int );

static void read_mp3_list_file ( songdata * list, const char *filename, int append )
{
	char *buf = NULL;
	char *file = NULL;
	FILE *fp;
	char *dir = NULL;
	char *playlistname = NULL;
	songdata_song *ftmp = NULL;
	int length = 0, n = 0, lines = 0;
	int playlist = 0;

	errno = 0;
	if ( ! ( fp = fopen ( filename, "r" ) ) )
		return;

	buf = calloc ( 1025, sizeof ( char ) );
	while ( !feof ( fp ) )
	{
		lines++;
		if(!fgets ( buf, 1024, fp )){
			break;
		}
		gui_progress_animate(); //TODO moet eigenlijk met timer van uit main...
	}
	fclose ( fp );
	errno = 0;
	if ( ! ( fp = fopen ( filename, "r" ) ) )
		return;

	if(!fgets ( buf, 1024, fp ))
		return;
	
	if ( !strncmp ( "Playlist for mjs", buf, 16 ) )
		playlist = 1;

	if ( playlist )
	{
		length = strrchr ( filename, '/' ) - filename;
		playlistname = malloc ( strlen ( filename ) - length - 4 );
        memset(playlistname, 0, strlen(filename) - length - 4);
		strncpy ( playlistname, filename + length + 1, strlen ( filename ) - length - 5 );
		playlistname[strlen ( filename ) - length - 5] = '\0';
	}

	if ( append | !list->head )
		songdata_clear ( list );

	if ( append )
	{
		ftmp = new_songdata_song();
		ftmp->flags |= F_DIR;
		ftmp->filename = strdup ( "../" );
		ftmp->fullpath = getcwd ( NULL, 0 );
		if ( playlist )
			ftmp->path = strdup ( playlistname );
		else
		{
			ftmp->path = strdup ( "Search Results" );
			list->flags |= F_VIRTUAL;
		}
		ftmp->relpath = strdup ( ftmp->path );
		songdata_add ( list, list->tail, ftmp );
	}
	else
	{
		list->flags |= F_VIRTUAL;
	}


	while ( !feof ( fp ) )
	{
		int pcts = (100 * n) / lines;
		if(pcts < 0)
		{
			pcts = 0;
		} 
		else if (pcts > 100)
		{
        	pcts = 100;
      	}
		n++;
        gui_progress_animate(); //TODO moet eigenlijk met timer van uit main...
        gui_progress_value(pcts);

		if ( !fgets ( buf, 1024, fp ) )
		{
			// end-of-file reached or got zero characters
			fclose ( fp );
			free ( buf );
			if ( playlistname )
				free ( playlistname );
			return;
		}
		//printf("%d - \n",n);
		length = strrchr ( buf, '/' ) - buf;
		if(length <= 0) continue;
		buf[strlen ( buf ) - 1] = '\0';	// Get rid off trailing newline
		
		dir = malloc ( length + 1 );
        memset(dir, 0, length + 1);
		
		file = malloc ( strlen ( buf ) - length );
        memset(file, 0, strlen(buf) - length);
		
		strncpy ( dir, buf, length );
		dir[length] = '\0';
		strcpy ( file, buf + length + 1 );
		if ( *filename == '.' )
			continue;

/* TODO What happens?? */
		if ( ( ftmp = mp3_info ( dir, file, playlistname, n ) ) )
			songdata_add_ordered( list, ftmp );

		free ( file );
		free ( dir );
	}
	fclose ( fp );
	free ( buf );
	if ( playlistname )
		free ( playlistname );

    gui_progress_stop();
	return;
}

/* Public functions */

void songdata_read_mp3_list ( songdata * list, const char * from, int append )
{
	struct stat st;
	if ( !lstat ( from, &st ) )
	{
		if ( S_ISLNK ( st.st_mode ) )
		{
			char tempdir[st.st_size + 1];

			//TODO: properly handle return value and potential failure.
#pragma GCC diagnostic ignored "-Wunused-result"
			readlink ( from, tempdir, st.st_size + 1);
	  		tempdir[st.st_size]='\0';
#pragma GCC diagnostic pop

			if(list->from)
				free(list->from);

			if(tempdir[0] == '/')
			{
    	  		list->from = strdup(tempdir);
			} 
			else
			{
				char * tmp_from = strdup(from);
				char * original_dirname = dirname(tmp_from);
				char * new_full_path = malloc(sizeof(char) * ( strlen(original_dirname) + strlen(tempdir) + 2));
				sprintf(new_full_path, "%s/%s", original_dirname, tempdir);
				free(tmp_from);
				list->from = new_full_path;
			}	
			stat ( list->from, &st );
		}
    	else
		{
			if(list->from){
				free(list->from);
			}
			list->from = strdup ( from );
		}

		if ( S_ISDIR ( st.st_mode ) )
		{
			disk_songdata_read_mp3_list_dir ( list, list->from, append );
		}
    	else
    	{
			switch ( append )
			{
			case L_SEARCH:
          		gui_progress_start(SEARCHING);
				break;
			default:
				gui_progress_start(READING);
				break;
      		}
      		read_mp3_list_file ( list, list->from, append );
      		gui_progress_stop();
    	}
	}
	return;
}

int songdata_save_playlist ( songdata * list, char *filename )
{
	FILE *fp;
	songdata_song *ftmp;

	if ( ! ( fp = fopen ( filename, "w" ) ) )
		return 1;
	
	fprintf ( fp, "Playlist for mjs\n" );
	
	for ( ftmp = list->head; ftmp; ftmp = ftmp->next )
		fprintf ( fp, "%s\n", ftmp->fullpath );
  
	fclose ( fp );
	return 0;
}

int songdata_check_file ( songdata_song * file )
{
	struct stat sb;

    if(!file)
		return 1;

	if ( !strncasecmp ( file->fullpath, "http", 4 ) )
		return 1;
	
	if ( stat ( file->fullpath, &sb ) == -1 )
	{
		char buf[1024];
		sprintf(buf, "Oeps: %s\n", file->fullpath);
		log_debug(buf);
		return 0;
    }
	else
	{
		return 1;
	}
}

/* find the next valid entry in the search direction */

songdata_song * songdata_next_valid ( songdata * list, songdata_song * file, int c )
{
	songdata_song *ftmp = NULL;
	if ( !file )
		return NULL;
	switch ( c )
	{
	case KEY_HOME:
	case KEY_DOWN:
	case KEY_NPAGE:
		while ( !songdata_check_file ( file ) )
		{
			if(!file->next) 
				break;
            
			ftmp = file->next;
			songdata_del ( list, file );
			file = songdata_next_valid ( list, ftmp, c );
		}
		break;
	case KEY_END:
	case KEY_UP:
	case KEY_PPAGE:
		while ( !songdata_check_file ( file ) )
		{
			ftmp = file->prev;
			songdata_del ( list, file );
			file = songdata_next_valid ( list, ftmp, c );
		}
		break;
	}
	
	return file;
}

songdata * songdata_init ( )
{
	colors = conf->colors;

    disk_songdata_init();

	mp3list = ( songdata * ) malloc( sizeof ( songdata ) );
    memset(mp3list, 0, sizeof(songdata));
	songdata_read_mp3_list ( mp3list, conf->mp3path, L_NEW );
	return mp3list;
}
void songdata_shutdown ( void )
{
    disk_songdata_shutdown();
    free(mp3list->from);
    songdata_clear(mp3list);
    free ( mp3list );
}

void songdata_randomize(songdata * list)
{
	int i = list->length, j, k;
	songdata_song *ftmp = NULL, *nesongdata = NULL, **farray = NULL;

	if ( i < 2 )
		return;
	
	if ( ! ( farray = ( songdata_song ** ) calloc ( i, sizeof ( songdata_song * ) ) ) )
		return;
	
	for ( ftmp = list->head, j = 0; ftmp; ftmp = ftmp->next, j++ )
		farray[j] = ftmp;

	k = ( int ) ( ( float ) i--*rand() / ( RAND_MAX+1.0 ) );
	nesongdata = farray[k];
	nesongdata->prev = NULL;
	farray[k] = NULL;
	list->head = list->top = nesongdata;

	for ( ftmp = NULL; i; i-- )
	{
		k = ( int ) ( ( float ) i*rand() / ( RAND_MAX+1.0 ) );
		for ( j = 0; j <= k; j++ )
		{
			if ( farray[j] == NULL )
				k++;
		}

		ftmp = farray[k];
		farray[k] = NULL;
		nesongdata->next = ftmp;
		if ( ftmp )
		{
			ftmp->prev = nesongdata;
			nesongdata = ftmp;
		}
	}
	
	list->selected = list->head;
	list->where = 1;
	list->wheretop = 0;
	list->tail = nesongdata;
	nesongdata->next = NULL;
	free ( farray );
}

//TODO move to song.c
songdata_song * new_songdata_song(void)
{
	songdata_song * newfile = malloc( sizeof ( songdata_song ) );
	memset(newfile, 0, sizeof(songdata_song));
	newfile->flags = 0;
	newfile->album = NULL;
	newfile->filename = NULL;		// filename without path
	newfile->path = NULL;		// path without filename without mp3path
	newfile->fullpath = NULL;		// the fullpath
	newfile->relpath = NULL;
	newfile->artist = NULL;
	newfile->genre = NULL;
	newfile->title = NULL;
	newfile->tag = NULL;
	newfile->track_id = 0;
	newfile->length = 0;
	newfile->catalog_id = 1;
	newfile->next = NULL;
	newfile->prev = NULL;
	return newfile;
}


void songdata_reload_search_results(){
	if ( ! ( mp3list->flags & F_VIRTUAL ) && mp3list->selected != NULL)
		dirstack_push (mp3list->from, mp3list->selected->filename );

	songdata_read_mp3_list ( mp3list, conf->resultsfile, L_SEARCH );
	gui_update_info();
	gui_update_filelist();
}

// these are NOT for external use, use songdata_clear instead
static void	free_list ( songdata_song * );
static void	free_songdata_song ( songdata_song * );
static int sort_songdata_song ( songdata_song *, songdata_song * );

/* sort directories first, then files. alphabetically of course. */
static int sort_songdata_song ( songdata_song *first, songdata_song *second )
{
	if ( first->filename[0] == '.' )
		return 1;
	if ( ( first->flags & ( F_DIR|F_PLAYLIST ) ) && ! ( second->flags & ( F_DIR|F_PLAYLIST ) ) )
		return 1;
	else if ( ! ( first->flags & ( F_DIR|F_PLAYLIST ) ) && ( second->flags & ( F_DIR|F_PLAYLIST ) ) )
		return -1;
	else
		return strcmp ( second->fullpath, first->fullpath );
}


void songdata_add_ordered ( songdata *list, songdata_song *new )
{
	songdata_song *ftmp = NULL;
	songdata_song *prev = NULL;
	for ( ftmp = list->head; ftmp; ftmp = ftmp->next ){
		if(sort_songdata_song(ftmp, new) < 0){
			//before ftmp so use prev as 'after' reference
			songdata_add(list, prev, new);
			return;
		}else{
			//After or the same, continue to find the correct prev
			prev = ftmp;
		}
	}
	if(!prev){
		//No prev found so use tail (last entry)
		prev = list->tail;
	}
	songdata_add(list, prev, new);
}

void songdata_add ( songdata *list, songdata_song *position, songdata_song *new )
{
  songdata_song *after;
  if ( !list )
    abort();

  list->length++;
  if ( list->selected == NULL )   //list was empty
  {
    list->head = list->tail = list->selected = new;
    list->where = 1;
    new->prev = new->next = NULL;
    return;
  }

	// if position == NULL add to front
  if ( position == NULL )
  {
    after = list->head;
    list->selected = list->head= new;
    after->prev = new;
    new->next = after;
    new->prev = NULL;
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
  if ( file->tag )
    free ( file->tag );
}
