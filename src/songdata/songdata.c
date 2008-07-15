#include "defs.h"
#include "songdata.h"

#include "mjs_id3.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <ncurses.h>


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
u_int32_t * colors;

wlist *mp3list;

static int sort_mp3 ( const void *, const void * );
static int sort_mp3_search ( const void *, const void * );

// these are NOT for external use, use wlist_clear instead
static void	free_list ( flist * );
static void	free_flist ( flist * );
static dirstack *dirstack_top = NULL;


void
wlist_add ( wlist *list, flist *position, flist *new )
{
	flist *after;
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
wlist_del ( wlist *list, flist *position )
{
	flist *before = position->prev, *after = position->next;
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



	free_flist ( position );
}

void
wlist_clear ( wlist *list )
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

void
free_list ( flist *list )
{
	flist *ftmp, *next;

	for ( ftmp = list; ftmp; )
	{
		next = ftmp->next;
		if ( ftmp )
		{
			free_flist ( ftmp );
			free ( ftmp );
		}
		ftmp = next;
	}
	//free(ftmp);
}

void
free_flist ( flist *file )
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

void
dirstack_push ( const char *fullpath, const char *filename )
{
	dirstack *tmp = calloc ( 1, sizeof ( dirstack ) );
	tmp->fullpath = strdup ( fullpath );
	tmp->filename = strdup ( filename );
	tmp->prev = dirstack_top;
	dirstack_top = tmp;
}

char *
dirstack_fullpath ( void )
{
	if ( dirstack_top )
		return dirstack_top->fullpath;
	else
		abort();
}

char *
dirstack_filename ( void )
{
	if ( dirstack_top )
		return dirstack_top->filename;
	else
		abort();
}

int
dirstack_empty ( void )
{
	if ( dirstack_top )
		return 0;
	else
		return 1;
}

void
dirstack_pop ( void )
{
	dirstack *tmp;
	if ( dirstack_top )
	{
		tmp = dirstack_top;
		free ( tmp->fullpath );
		free ( tmp->filename );
		dirstack_top = tmp->prev;
		free ( tmp );
	}
	else
		abort();
}



__inline__
char *
split_filename ( char **s )
{
	char *part;
	char *end;
	int length;
	if ( !*s )
		return NULL;
	if ( ( *s ) [0]=='/' )
		( *s ) ++;
	if ( ( end = strchr ( *s, '/' ) ) )
		length = end - *s;
	else
		length = strlen ( *s );
	part = calloc ( length + 1, sizeof ( char ) );
	strncpy ( part, *s, length );
	part[length] = '\0';
	*s = end;
	return part;
}



flist *
mp3_info ( const char *abspath, const char *filename, const char *playlistname, int count )
{
	flist *ftmp = NULL;
	char *fullpath, *path, newpath[256];
	struct stat st;
	unsigned int length;
	int filename_len = strlen ( filename );

	// look whether a symlink is part of the filename
	if ( conf->c_flags & C_USE_GENRE )
	{
		struct stat st;
		char *sympath;
		sympath = strdup ( abspath );
		lstat ( sympath, &st );

		/* FIXME --> (strlen(sympath)>9 ) */

		while ( ( !S_ISLNK ( st.st_mode ) ) && ( strlen ( sympath ) >9 ) )
		{
			*strrchr ( sympath, '/' ) = '\0';
			lstat ( sympath, &st );
		}
		if ( S_ISLNK ( st.st_mode ) )
		{

			char tempdir[256];
			int n;
			n = readlink ( sympath, tempdir, sizeof ( tempdir ) );
			tempdir[n]='\0';
			strncpy ( newpath, tempdir, sizeof ( newpath ) );
			strncat ( newpath, abspath + strlen ( sympath ), sizeof ( newpath ) - strlen ( newpath ) );
			abspath = newpath;
		}
		free ( sympath );
	}




	fullpath = calloc ( strlen ( abspath ) + filename_len + 2, sizeof ( char ) );
	sprintf ( fullpath, "%s/%s", abspath, filename );
	if ( ( length = strlen ( conf->mp3path ) ) == strlen ( abspath ) + 1 )
		path = "\0";
	else
	{
		//TODO is dit nog steeds correct?
		path = strdup ( abspath );
		path += strlen ( conf->mp3path );
	}
	if ( path[0]=='/' )
		path++;

	if ( filename[0]=='.' )
	{
		ftmp = calloc ( 1, sizeof ( flist ) );
		ftmp->flags |= F_DIR;
		ftmp->filename = strdup ( "../" );
		ftmp->fullpath = strdup ( "../" );
		ftmp->path = strdup ( path );
		ftmp->relpath = strdup ( path );
		ftmp->has_id3 = 0;
		return ftmp;
	}


	if ( !stat ( fullpath, &st ) )
	{

		if ( S_ISDIR ( st.st_mode ) )
		{
//directory
			ftmp = calloc ( 1, sizeof ( flist ) );
			ftmp->path = calloc ( strlen ( path ) + filename_len + 2, sizeof ( char ) );
			sprintf ( ftmp->path, "%s/%s", path, filename );
			ftmp->fullpath = strdup ( fullpath );
			ftmp->relpath = strdup ( path );
			ftmp->flags |= F_DIR;

			ftmp->filename = calloc ( filename_len + 2, sizeof ( char ) );
			strcpy ( ftmp->filename, filename );
			if ( ( count!=0 ) & ( !playlistname ) )
				strcat ( ftmp->filename, ":" );
			else
				strcat ( ftmp->filename, "/" );
			ftmp->title = strdup ( ftmp->filename );
			ftmp->genre = NULL;
			ftmp->album = NULL;
			ftmp->artist = NULL;
			ftmp->has_id3 = 0;


		}
		else if ( S_ISREG ( st.st_mode ) )
		{
			if ( ( strncasecmp ( ".mp3", strchr ( filename, '\0' ) - 4, 4 ) ) && ( strncasecmp ( ".mjs", strchr ( filename, '\0' ) - 4, 4 ) ) )
				return NULL;

			ftmp = calloc ( 1, sizeof ( flist ) );
			ftmp->path = strdup ( path );
			ftmp->relpath = NULL;
			ftmp->fullpath = strdup ( fullpath );
			ftmp->album = NULL;
			ftmp->artist = NULL;
			ftmp->has_id3 = 0;

			if ( !strncasecmp ( ".mp3", strchr ( filename, '\0' ) - 4, 4 ) )
			{
// mp3-file
				if ( conf->c_flags & C_USE_GENRE )
					ftmp->genre = split_filename ( &path );

				ftmp->artist = split_filename ( &path );

				if ( playlistname )
				{
					int l;
					ftmp->album = strdup ( playlistname );
					// get rid of old tracknumber add new tracknumber
					if ( ( filename[0] >= '0' ) & ( filename[0] <= '9' ) )
						filename = filename + 3;
					filename_len = strlen ( filename ) - 4;
					ftmp->filename = malloc ( filename_len + 6 );
					l = snprintf ( ftmp->filename, 6, "%02.0f ", ( float ) count );
					strncat ( ftmp->filename, filename, filename_len );
				}
				else
				{
					ftmp->filename = calloc ( filename_len - 3, sizeof ( char ) );
					strncpy ( ftmp->filename, filename, filename_len - 4 );
					ftmp->filename[filename_len - 4] = '\0';

					if ( path )
						ftmp->album = strdup ( path + 1 );
					else
						ftmp->album = NULL;
				}
				ftmp->title = strdup ( ftmp->filename );
				//id3tag_t tag;
				FILE * file = fopen ( fullpath, "r" );
				mp3info mp3;
				memset ( &mp3,0,sizeof ( mp3info ) );
				mp3.filename = fullpath;
				mp3.file = file;
				if ( get_mp3_info ( &mp3, 1 ) == 0 && mp3.id3_isvalid == 1 )
				{
					id3tag tag = mp3.id3;
					if ( tag.title != NULL && tag.artist != NULL && strcmp ( tag.title, "" ) != 0 && strcmp ( tag.artist, "" ) != 0 && id3_isvalidtag ( tag ) )
					{
						if ( ftmp->title != NULL )
							free ( ftmp->title );
						if ( ftmp->artist != NULL )
							free ( ftmp->artist );
						if ( ftmp->album != NULL )
							free ( ftmp->album );
						if ( ftmp->genre != NULL )
							free ( ftmp->genre );

						ftmp->title = strdup ( tag.title );
						ftmp->artist = strdup ( tag.artist );
						ftmp->album = strdup ( tag.album );
						ftmp->track_id = ( int ) tag.track;
						ftmp->length = mp3.seconds;
						ftmp->genre = strdup ( id3_findstyle ( tag.genre[0] ) );
						ftmp->has_id3 = 1;
						ftmp->filename = calloc ( strlen ( ftmp->title ) + strlen ( ftmp->artist ) + 4, sizeof ( char ) );
						sprintf ( ftmp->filename, "%s - %s", ftmp->artist, ftmp->title );

					}
				}
				fclose ( file );
			}
			else
			{
// playlist-file
				ftmp->flags |= F_PLAYLIST;
				ftmp->filename = calloc ( strlen ( filename ) - 2, sizeof ( char ) );
				strncpy ( ftmp->filename , filename, filename_len - 4 );
				strcat ( ftmp->filename, "/\0" );
				ftmp->title = strdup ( ftmp->filename );
				ftmp->album = NULL;
				ftmp->artist = NULL;
				ftmp->has_id3 = 0;
			}
		}
	}
	else
	{
		if ( !strncasecmp ( fullpath, "http", 4 ) )
		{
			// web-cast http adres
			ftmp = calloc ( 1, sizeof ( flist ) );
			ftmp->flags |= F_HTTP;
			ftmp->filename = calloc ( strlen ( playlistname ) + 11, sizeof ( char ) );
			strcpy ( ftmp->filename, "WebRadio: \0" );
			strcat ( ftmp->filename, playlistname );
			ftmp->title = strdup ( ftmp->filename );
			ftmp->album = strdup ( fullpath );
			ftmp->artist = strdup ( "http-stream" );
			ftmp->fullpath = strdup ( fullpath );
			ftmp->path = strdup ( "http-stream" );
			ftmp->has_id3 = 0;
		}
	}
	free ( fullpath );
	return ftmp;

}






void
read_mp3_list ( wlist * list, const char * from, int append )
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
			read_mp3_list_dir ( list, list->from, append );
			switch ( append )
			{
				case L_NEW:
					if ( list->head )
						sort_songs ( list );
					break;
				case L_SEARCH:
					if ( list->head )
						sort_search ( list );
					break;
			}
		}
		else
		{
			switch ( append )
			{
				case L_SEARCH:
					window_menubar_deactivate();
					printf_menubar ( SEARCHING );
					break;
				default:
					window_menubar_deactivate();
					printf_menubar ( READING );
			}
			read_mp3_list_file ( list, list->from, append );
			if ( ( append & L_SEARCH ) && ( list->head ) )
				sort_search ( list );
			window_menubar_activate();
		}
	}
	return;
}


void
read_mp3_list_dir ( wlist * list, const char * directory, int append )
{
	char *dir = NULL;
	DIR *dptr = NULL;
	struct dirent *dent;
	flist *ftmp = NULL;

	dir = strdup ( directory );

	wlist_clear ( list );

	chdir ( dir );


	if ( ( strncmp ( dir, conf->mp3path, strlen ( conf->mp3path )-1 ) ) && ( strcmp ( dir, conf->playlistpath ) ) )
	{
		/* TODO uitgezet om problemen met nieuwe indeling te voorkomen*/
		/*chdir (conf->mp3path);
		dir = strdup(conf->mp3path);*/
		ftmp = mp3_info ( dir, "../", NULL, 0 );
		wlist_add ( list, list->tail, ftmp );
	}
	else if ( !dirstack_empty() )
	{
		ftmp = mp3_info ( dir, "../", NULL, 0 );
		wlist_add ( list, list->tail, ftmp );
	}


	dptr = opendir ( dir );
	while ( ( dent = readdir ( dptr ) ) )
	{
		if ( *dent->d_name == '.' )
			continue;

		if ( ( ftmp = mp3_info ( dir, dent->d_name, NULL, 0 ) ) )
			wlist_add ( list, list->tail, ftmp );
	}
	closedir ( dptr );
	free ( dir );
	return;
}

int
write_mp3_list_file ( wlist * list, char *filename )
{
	FILE *fp;
	flist *ftmp;

	if ( ! ( fp = fopen ( filename, "w" ) ) )
		return 1;
	fprintf ( fp, "Playlist for mjs\n" );
	for ( ftmp = list->head; ftmp; ftmp = ftmp->next )
		fprintf ( fp, "%s\n", ftmp->fullpath );
	fclose ( fp );
	return 0;
}


void
read_mp3_list_file ( wlist * list, const char *filename, int append )
{
	char *buf = NULL;
	char *file = NULL;
	FILE *fp;
	char *dir = NULL;
	char *playlistname = NULL;
	flist *ftmp = NULL;
	int length = 0, n = 0, lines = 0;
	int playlist = 0;

	errno = 0;
	if ( ! ( fp = fopen ( filename, "r" ) ) )
		return;

	buf = calloc ( 256, sizeof ( char ) );
	while ( !feof ( fp ) )
	{
		lines++;
		fgets ( buf, 255, fp );
	}
	fclose ( fp );
	errno = 0;
	if ( ! ( fp = fopen ( filename, "r" ) ) )
		return;

	fgets ( buf, 255, fp );
	if ( !strncmp ( "Playlist for mjs", buf, 16 ) )
		playlist = 1;

	if ( playlist )
	{
		length = strrchr ( filename, '/' ) - filename;
		playlistname = malloc ( strlen ( filename ) - length - 4 );
		strncpy ( playlistname, filename + length + 1, strlen ( filename ) - length - 5 );
		playlistname[strlen ( filename ) - length - 5] = '\0';
	}

	if ( append | !list->head )
		wlist_clear ( list );

	if ( append )
	{
		ftmp = calloc ( 1, sizeof ( flist ) );
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
		wlist_add ( list, list->tail, ftmp );
	}
	else
		list->flags |= F_VIRTUAL;


	while ( !feof ( fp ) )
	{
		n++;
		/*      my_mvwaddstr (menubar->win, 0, (26 + (n * (50 / (float)lines))), colors[MENU_TEXT], "*");*/
		//TODO wat hier staat moet in een progressbar functie komen.....
		if ( ( n & 0x03 ) ==0x00 )
		{
			// my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "|"); //TODO wat hier staat moet in een progressbar functie komen.....
		}
		else
			if ( ( n & 0x03 ) ==0x01 )
			{
				// my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "/");//TODO wat hier staat moet in een progressbar functie komen.....
			}
			else
				if ( ( n & 0x03 ) ==0x02 )
				{
					//  my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "-");//TODO wat hier staat moet in een progressbar functie komen.....
				}
				else
					// my_mvwaddstr (menubar->win, 0, 79 , colors[MENU_TEXT], "\\");//TODO wat hier staat moet in een progressbar functie komen.....
					update_panels ();
		doupdate ();

		if ( !fgets ( buf, 255, fp ) )
		{
			// end-of-file reached or got zero characters
			fclose ( fp );
			free ( buf );
			if ( playlistname )
				free ( playlistname );
			return;
		}
		length = strrchr ( buf, '/' ) - buf;
		buf[strlen ( buf ) - 1] = '\0';	// Get rid off trailing newline
		dir = malloc ( length + 1 );
		file = malloc ( strlen ( buf ) - length );
		strncpy ( dir, buf, length );
		dir[length] = '\0';
		strcpy ( file, buf + length + 1 );
		if ( *filename == '.' )
			continue;

		if ( ( ftmp = mp3_info ( dir, file, playlistname, n ) ) )
			wlist_add ( list, list->tail, ftmp );

		free ( file );
		free ( dir );
	}
	fclose ( fp );
	free ( buf );
	if ( playlistname )
		free ( playlistname );
	return;
}

void
read_mp3_list_array ( wlist * list, int argc, char *argv[] )
{
	return;
}
/* sort directories first, then files. alphabetically of course. */
wlist *
sort_songs ( wlist * sort )
{
	int i = 0, j;
	flist *ftmp = NULL, *newlist = NULL, **fsort;

	for ( ftmp = sort->head; ftmp; ftmp = ftmp->next )
		i++;
	fsort = ( flist ** ) calloc ( i, sizeof ( flist * ) );
	for ( ftmp = sort->head, j = 0; ftmp; ftmp = ftmp->next, j++ )
		fsort[j] = ftmp;

	qsort ( ( void * ) fsort, i, sizeof ( flist * ), sort_mp3 );
	sort->tail = newlist = fsort[0];
	newlist->next = NULL;

	for ( j = 1; j < i; j++ )
	{
		ftmp = fsort[j];
		ftmp->next = newlist;
		newlist->prev = ftmp;
		newlist = ftmp;
	}
	free ( fsort );
	newlist->prev = NULL;
	sort->head = sort->top = sort->selected = newlist;
	return sort;
}

wlist *
sort_search ( wlist * sort )
{
	int i = 0, j;
	flist *ftmp = NULL, *newlist = NULL, **fsort;

	for ( ftmp = sort->head; ftmp; ftmp = ftmp->next )
		i++;
	fsort = ( flist ** ) calloc ( i, sizeof ( flist * ) );
	for ( ftmp = sort->head, j = 0; ftmp; ftmp = ftmp->next, j++ )
		fsort[j] = ftmp;

	qsort ( ( void * ) fsort, i, sizeof ( flist * ), sort_mp3_search );
	sort->tail = newlist = fsort[0];
	newlist->next = NULL;

	for ( j = 1; j < i; j++ )
	{
		ftmp = fsort[j];
		ftmp->next = newlist;
		newlist->prev = ftmp;
		newlist = ftmp;
	}
	free ( fsort );
	newlist->prev = NULL;
	sort->head = sort->top = sort->selected = newlist;
	return sort;
}

static int
sort_mp3 ( const void *a, const void *b )
{
	const flist *first = * ( const flist ** ) a;
	const flist *second = * ( const flist ** ) b;

	if ( first->filename[0] == '.' )
		return 1;
	if ( ( first->flags & ( F_DIR|F_PLAYLIST ) ) && ! ( second->flags & ( F_DIR|F_PLAYLIST ) ) )
		return 1;
	else if ( ! ( first->flags & ( F_DIR|F_PLAYLIST ) ) && ( second->flags & ( F_DIR|F_PLAYLIST ) ) )
		return -1;
	else
		return strcmp ( second->fullpath, first->fullpath );
}

static int
sort_mp3_search ( const void *a, const void *b )
{
	const flist *first = * ( const flist ** ) a;
	const flist *second = * ( const flist ** ) b;
	int result;

	if ( first->filename[0] == '.' )
		return 1;
	if ( ! ( result = strcmp ( second->path, first->path ) ) )
	{
		if ( ( first->flags & F_DIR ) && ! ( second->flags & F_DIR ) )
			return 1;
		else if ( ! ( first->flags & F_DIR ) && ( second->flags & F_DIR ) )
			return -1;
		else
			return strcmp ( second->filename, first->filename );
	}
	return result;
}

__inline__ int
check_file ( flist * file )
{
	struct stat sb;

	if ( !strncasecmp ( file->fullpath, "http", 4 ) )
		return 1;
	if ( stat ( file->fullpath, &sb ) == -1 )
		return 0;
	else
		return 1;
}

/* find the next valid entry in the search direction */

flist *
next_valid ( wlist * list, flist * file, int c )
{
	flist *ftmp = NULL;
	if ( !file )
		return NULL;
	switch ( c )
	{
		case KEY_HOME:
		case KEY_DOWN:
		case KEY_NPAGE:
			while ( !check_file ( file ) )
			{
				ftmp = file->next;
				wlist_del ( list, file );
				file = next_valid ( list, ftmp, c );
			}
			break;
		case KEY_END:
		case KEY_UP:
		case KEY_PPAGE:
			while ( !check_file ( file ) )
			{
				ftmp = file->prev;
				wlist_del ( list, file );
				file = next_valid ( list, ftmp, c );
			}
			break;
	}
	return file;

}

wlist * songdata_init ( Config * init_conf, u_int32_t init_colors[] )
{
	conf = init_conf;
	colors = init_colors;
	mp3list = ( wlist * ) calloc ( 1, sizeof ( wlist ) );
	read_mp3_list ( mp3list, conf->mp3path, L_NEW );
	return mp3list;
}
void songdata_shutdown ( void )
{
	free ( mp3list );
}

void songdata_randomize(wlist * list)
{
  int i = list->length, j, k;
  flist *ftmp = NULL, *newlist = NULL, **farray = NULL;

  if ( i < 2 )
    return;
  if ( ! ( farray = ( flist ** ) calloc ( i, sizeof ( flist * ) ) ) )
    return;
  for ( ftmp = list->head, j = 0; ftmp; ftmp = ftmp->next, j++ )
    farray[j] = ftmp;
  k = ( int ) ( ( float ) i--*rand() / ( RAND_MAX+1.0 ) );
  newlist = farray[k];
  newlist->prev = NULL;
  farray[k] = NULL;
  list->head = list->top = newlist;
  for ( ftmp = NULL; i; i-- )
  {
    k = ( int ) ( ( float ) i*rand() / ( RAND_MAX+1.0 ) );
    for ( j = 0; j <= k; j++ )
      if ( farray[j] == NULL )
        k++;
    ftmp = farray[k];
    farray[k] = NULL;
    newlist->next = ftmp;
    if ( ftmp )
    {
      ftmp->prev = newlist;
      newlist = ftmp;
    }
  }
  list->selected = list->head;
  list->where = 1;
  list->wheretop = 0;
  list->tail = newlist;
  newlist->next = NULL;
  free ( farray );
}
