#include "defs.h"
#include "controller.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "gui/gui.h"
#include "gui/inputline.h"
#include "mjs.h"
#include "log.h"
#include "keyboard_controller.h"
#include <string.h>


Config * conf;
wlist * playlist;

static struct sigaction handler;

static int do_save ( Input * );
static int do_search ( Input * );
static void controller_update_whereplaying ( void );
static void controller_update_statefile ( void );

void play_next_song()
{
	flist *ftmp = playlist->playing;

	if ( !ftmp )
		return;

	if ( ( conf->c_flags & C_LOOP ) && !ftmp->next )
		ftmp = playlist->head;
	else
		ftmp = ftmp->next;
	controller_jump_to_song ( ftmp );
}

void play_prev_song()
{
	flist *ftmp = playlist->playing;

	if ( !ftmp )
		return;
	if ( ( conf->c_flags & C_LOOP ) && !ftmp->prev )
		ftmp = playlist->tail;
	else
		ftmp = ftmp->prev;
	controller_jump_to_song ( ftmp );
}

static void controller_update_whereplaying ( void )
{
	flist *ftmp;
	for ( ftmp = playlist->head, playlist->whereplaying=0; ftmp!=playlist->playing; ftmp=ftmp->next )
		playlist->whereplaying++;
}

static void controller_update_statefile ( void )
{
	FILE *activefile;
	activefile = fopen ( conf->statefile,"w" );
	if ( activefile )
	{
		if ( playlist->playing != NULL )
		{
			fprintf ( activefile,"         Now playing:  %s  (by)  %s  (from)  %s    \n", playlist->playing->filename, playlist->playing->artist, playlist->playing->album );
		}
		else
		{
			fprintf ( activefile,"%s","                      \n" );
		}
		fclose ( activefile );
	}
}

void controller_process_to_next_song ( void )
{
	if ( !playlist->playing->next )
		return;

	playlist->playing = playlist->playing->next;

	/* GUI stuff */
	window_play_update();
	window_info_update();
	window_playback_update();

	controller_update_whereplaying();
	controller_update_statefile();
}

void controller_jump_to_song ( flist *next )
{
	if ( !next )
		return;

	if ( playlist->playing != next )
	{
		//It's really a jump....
		engine_stop();
	}

	playlist->playing = next;

	engine_play();

	/* GUI stuff */
	window_play_update();
	window_info_update();
	window_playback_update();

	controller_update_whereplaying();
	controller_update_statefile();
}

void controller_stop()
{
	if ( playlist->playing )
	{
		playlist->playing->flags &= ~F_PAUSED;
		playlist->playing = NULL;
		window_play_update();
	}
	engine_stop();
	window_playback_update();
	controller_update_statefile();
}

void
add_to_playlist_recursive ( wlist *list, flist *position, flist *file )
{
	char *prevpwd = NULL;
	wlist *templist = NULL;
	if ( ! ( file->flags & F_DIR ) )
		return;

	templist = calloc ( 1, sizeof ( wlist ) );
	prevpwd = getcwd ( NULL, 0 );

	read_mp3_list ( templist, file->fullpath, L_NEW );
	if ( !strncmp ( templist->selected->filename, "../", 3 ) )
		templist->selected = templist->head->next; // skip ../ entry

	while ( templist->selected )
	{
		if ( templist->selected->flags & F_DIR )
			add_to_playlist_recursive ( list, list->tail, templist->selected );
		else if ( ! ( templist->selected->flags & F_PLAYLIST ) )
			add_to_playlist ( list, list->tail, templist->selected );

		templist->selected = next_valid ( templist, templist->selected->next, KEY_DOWN );
	}

	wlist_clear ( templist );
	free ( templist );
	chdir ( prevpwd );
	free ( prevpwd );
}

void
add_to_playlist ( wlist *list, flist *position, flist *file )
{
	flist *newfile;
	char *p;

	if ( !check_file ( file ) )
		return;
	newfile = calloc ( 1, sizeof ( flist ) );

	/* remove tracknumber if it exists and user wants it*/
	if ( ! ( conf->c_flags & C_TRACK_NUMBERS ) )
	{
		if ( ( file->filename[0]>='0' ) & ( file->filename[0]<='9' ) )
		{
			if ( ( p = strchr ( file->filename, ' ' ) ) )
				newfile->filename = strdup ( p + 1 );
		}
		else if ( !strncasecmp ( file->filename,"cd",2 ) )
			newfile->filename = strdup ( file->filename+7 );
	}

	if ( !newfile->filename )
		newfile->filename = strdup ( file->filename );
	if ( strlen ( newfile->filename ) == 0 )
	{
		free ( newfile->filename );
		newfile->filename = strdup ( "..." );
	}

	newfile->path = strdup ( file->path );

	newfile->fullpath = strdup ( file->fullpath );

	if ( file->genre )
		newfile->genre = strdup ( file->genre );

	if ( file->album )
		newfile->album = strdup ( file->album );

	if ( file->artist )
		newfile->artist = strdup ( file->artist );

	if ( file->title )
		newfile->title = strdup ( file->title );

	newfile->has_id3 = file->has_id3;
	newfile->track_id = file->track_id;
	newfile->length = file->length;

	wlist_add ( list, position, newfile );

	if ( conf->c_flags & C_PADVANCE )
	{
		list->selected = newfile;
		list->where = list->length;
	}
	return;
}

wlist * controller_init (Config * init_config)
{
	conf = init_config;
	playlist = malloc ( sizeof ( wlist ) );
	playlist->head = NULL;
	wlist_clear ( playlist );
    keyboard_controller_init(playlist, conf);
	return playlist;
}

void controller_shutdown ( void )
{
  keyboard_controller_shutdown();
	free ( playlist );
}


static int
do_save ( Input * input )
{
	char *s = malloc ( strlen ( input->buf ) + 26 );
//  active = old_active;  //TODO anders
	sprintf ( s, "%s/%s.mjs", conf->playlistpath, input->buf );
	write_mp3_list_file ( playlist, s );
	free ( s );
	free ( input );
	window_menubar_activate();
	/*  menubar->inputline = NULL;*/ //TODO op nieuwe manier aanroepen
	doupdate ();
	return 1;
}

static int
do_search ( Input * input )
{
	pid_t childpid;
//  active = old_active;  //TODO anders
	if ( ! ( ( *input->buf == ' ' ) || ( *input->buf == '\0' ) ) )
	{
		handler.sa_handler = SIG_DFL;
		handler.sa_flags = SA_ONESHOT;
		sigaction ( SIGCHLD, &handler, NULL );
		errno = 0;
		if ( ! ( childpid = fork () ) )
		{
			errno = 0;
			execlp ( "findmp3", "findmp3", input->buf, conf->resultsfile, ( char * ) NULL );
			exit ( 3 );
		}
		if ( errno )
			exit ( 3 );

		waitpid ( childpid, NULL, 0 );

		handler.sa_handler = ( SIGHANDLER ) unsuspend;
		handler.sa_flags = SA_RESTART;
		sigaction ( SIGCONT, &handler, NULL );

/*		if ( ! ( mp3list->flags & F_VIRTUAL ) ) //TODO anders oplossen!!
			dirstack_push ( mp3list->from, mp3list->selected->filename ); 
        read_mp3_list ( mp3list, conf->resultsfile, L_SEARCH );*/
		window_files_update();
		window_info_update();
	}
	else
		window_menubar_activate();

	free ( input );
	//menubar->inputline = NULL;  //TODO anders
	window_files_update();
	return 1;
}


void
controller_playlist_move_up()
{
	flist *f1,*f2,*f3,*f4;

	f3 = playlist->selected;
	f2 = f3->prev;
	f1 = f2->prev;
	f4 = f3->next;

	f3->prev = f1;
	if ( f1 )
		f1->next = f3;
	else
	{
		playlist->head = f3;
		playlist->top = f3;
	}
	f3->next = f2;
	f2->prev = f3;
	f2->next = f4;
	if ( f4 )
		f4->prev = f2;
	else
		playlist->tail = f2;
	playlist->where--;
	return;
}

void
controller_playlist_move_down()
{
	flist *f1,*f2,*f3,*f4;

	f2 = playlist->selected;
	f3 = f2->next;
	f1 = f2->prev;
	f4 = f3->next;

	if ( f1 )
		f1->next = f3;
	else
	{
		playlist->head = f3;
		playlist->top = f3;
	}
	f3->prev = f1;
	f3->next = f2;
	f2->prev = f3;
	f2->next = f4;
	if ( f4 )
		f4->prev = f2;
	else
		playlist->tail = f2;
	playlist->where++;
	return;
}


