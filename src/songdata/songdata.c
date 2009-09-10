#include "defs.h"
#include "songdata.h"
#include "disk_songdata.h"
#include "mysql_songdata.h"
#include "log.h"



#include "gui/window_info.h"
#include "gui/window_files.h"



#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ncurses.h>

#include "gui/window_menubar.h"


/* 	path -> path including trailing slash
	filename -> filename without any slashes
	count -> pointer to int

	mp3path = /pub/mp3/

	|*************************fullpath*****************************|
	/pub/mp3/Pop/Cranberries/Bury the hatchet/01 Animal instinct.mp3
	         |*************path*************| |*****filename***|
		 |*| |*Artist**| |*****Album****|
		  |
		  \-Genre

	|**************abspath**************|

	|***************fullpath*************|
	/pub/mp3/Cranberries/Bury the hatchet/
	         |************path**********|
			     |***filename***|
		|*relpath**|
	|******abspath*****|
*/

Config * conf;
/* colors */
int * colors;

songdata *mp3list;

static int sort_mp3 ( const void *, const void * );
static songdata	*sort_songs ( songdata * );
static void	read_mp3_list_file ( songdata *, const char *, int );


static void
read_mp3_list_file ( songdata * list, const char *filename, int append )
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
		fgets ( buf, 1024, fp );
        window_menubar_progress_bar_animate(); //TODO moet eigenlijk met timer van uit main...
	}
	fclose ( fp );
	errno = 0;
	if ( ! ( fp = fopen ( filename, "r" ) ) )
		return;

	fgets ( buf, 1024, fp );
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
		list->flags |= F_VIRTUAL;


	while ( !feof ( fp ) )
	{
      int pcts = (100 * n) / lines;
      if(pcts < 0){
        pcts = 0;
      }else if(pcts > 100){
        pcts = 100;
      }
		n++;
        window_menubar_progress_bar_animate(); //TODO moet eigenlijk met timer van uit main...
        window_menubar_progress_bar_progress(pcts);

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

/* TODO What happens??         
		if ( ( ftmp = mp3_info ( dir, file, playlistname, n ) ) )
			songdata_add ( list, list->tail, ftmp );*/

		free ( file );
		free ( dir );
	}
	fclose ( fp );
	free ( buf );
	if ( playlistname )
		free ( playlistname );
    
    window_menubar_progress_bar_remove();
	return;
}

/* sort directories first, then files. alphabetically of course. */
static songdata *
sort_songs ( songdata * sort )
{
	int i = 0, j;
	songdata_song *ftmp = NULL, *nesongdata = NULL, **fsort;

	for ( ftmp = sort->head; ftmp; ftmp = ftmp->next )
		i++;
	fsort = ( songdata_song ** ) calloc ( i, sizeof ( songdata_song * ) );
	for ( ftmp = sort->head, j = 0; ftmp; ftmp = ftmp->next, j++ )
		fsort[j] = ftmp;

	qsort ( ( void * ) fsort, i, sizeof ( songdata_song * ), sort_mp3 );
	sort->tail = nesongdata = fsort[0];
    if(!nesongdata){
      return sort;
    }
	nesongdata->next = NULL;

	for ( j = 1; j < i; j++ )
	{
		ftmp = fsort[j];
		ftmp->next = nesongdata;
		nesongdata->prev = ftmp;
		nesongdata = ftmp;
	}
    if(fsort != NULL)
	 free ( fsort );
	nesongdata->prev = NULL;
	sort->head = sort->top = sort->selected = nesongdata;
	return sort;
}

/* Private Functions */

static int sort_mp3 ( const void *a, const void *b )
{
	const songdata_song *first = * ( const songdata_song ** ) a;
	const songdata_song *second = * ( const songdata_song ** ) b;

	if ( first->filename[0] == '.' )
		return 1;
	if ( ( first->flags & ( F_DIR|F_PLAYLIST ) ) && ! ( second->flags & ( F_DIR|F_PLAYLIST ) ) )
		return 1;
	else if ( ! ( first->flags & ( F_DIR|F_PLAYLIST ) ) && ( second->flags & ( F_DIR|F_PLAYLIST ) ) )
		return -1;
	else
		return strcmp ( second->fullpath, first->fullpath );
}

/* Public functions */

void songdata_read_mp3_list ( songdata * list, const char * from, int append )
{
  struct stat st;
  if ( !lstat ( from, &st ) )
  {

    if ( S_ISLNK ( st.st_mode ) )
    {
      char tempdir[256];
      int n;
      n = readlink ( from, tempdir, sizeof ( tempdir ) );
      tempdir[n]='\0';

      list->from = strdup ( tempdir );
      stat ( list->from, &st );
    }
    else
      list->from = strdup ( from );


    if ( S_ISDIR ( st.st_mode ) )
    {
      mysql_songdata_read_mp3_list_dir ( list, list->from, append ); //TODO moet nog kunnen wisselen
      sort_songs ( list );
    }
    else
    {
      switch ( append )
      {
        case L_SEARCH:
          window_menubar_progress_bar_init(SEARCHING);
          break;
        default:
          window_menubar_progress_bar_init(READING);
      }
      read_mp3_list_file ( list, list->from, append );
      if ( ( append & L_SEARCH ) && ( list->head ) )
        sort_songs ( list );
      window_menubar_activate();
    }
  }
  return;
}

int
songdata_save_playlist ( songdata * list, char *filename )
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

int
songdata_check_file ( songdata_song * file )
{
	struct stat sb;

    if(!file){
      return 1;
    }
	if ( !strncasecmp ( file->fullpath, "http", 4 ) )
		return 1;
	if ( stat ( file->fullpath, &sb ) == -1 ){
      char buf[1024];
      sprintf(buf, "Oeps: %s\n", file->fullpath);
      log_debug(buf);
		return 0;
    }
	else
		return 1;
}

/* find the next valid entry in the search direction */

songdata_song *
songdata_next_valid ( songdata * list, songdata_song * file, int c )
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
                if(!file->next){
                  break;
                }
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

songdata * songdata_init ( Config * init_conf, int init_colors[] )
{
	conf = init_conf;
	colors = init_colors;
    log_init();
    //TODO wisselen
    mysql_songdata_init(init_conf);
    
    
	mp3list = ( songdata * ) malloc( sizeof ( songdata ) );
    memset(mp3list, 0, sizeof(songdata));
	songdata_read_mp3_list ( mp3list, conf->mp3path, L_NEW );
	return mp3list;
}
void songdata_shutdown ( void )
{
    mysql_songdata_shutdown(); //TODO wisselen
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
      if ( farray[j] == NULL )
        k++;
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
  newfile->has_id3 = 0;
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
  window_info_update();
  window_files_update();
}
