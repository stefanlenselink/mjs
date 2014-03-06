#include "defs.h"
#include "controller.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "gui/gui.h"
#include "mjs.h"
#include "log.h"
#include "config/config.h"
#include "http_controller.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>


extern Config * conf;
songdata * playlist;

static struct sigaction handler;
static FILE *logfile;

static void controller_update_whereplaying ( void );
static void controller_update_statefile ( void );

void controller_next( void )
{
	songdata_song *ftmp = playlist->playing;

	if ( !ftmp )
		return;

	if ( ( conf->c_flags & C_LOOP ) && !ftmp->next )
		ftmp = playlist->head;
	else
		ftmp = ftmp->next;
	controller_jump_to_song ( ftmp );
}

void controller_prev( void )
{
	songdata_song *ftmp = playlist->playing;

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
	songdata_song *ftmp;
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
 
char * controller_process_to_next_song ( void )
{
	time_t timevalue;

	if (logfile) {
		timevalue = time(NULL);
		fprintf(logfile, "%.24s %s\n", ctime(&timevalue), playlist->playing->fullpath);
		fsync(fileno(logfile));
	}

    if ( !playlist->playing->next )
		return NULL;
    char * return_path = playlist->playing->next->fullpath;
	playlist->playing = playlist->playing->next;
   // if( playlist->playing->next)
   // {
   //  return_path = playlist->playing->next->fullpath;
   // }
	/* GUI stuff */
	gui_update_playlist();
	gui_update_info();
	gui_update_playback();

	controller_update_whereplaying();
	controller_update_statefile();
    return return_path;
}

void controller_jump_to_song ( songdata_song *next )
{
	if ( !next )
		return;

	if ( playlist->playing != next )
	{
		//It's really a jump....
		engine_stop();
	}
    char * current = next->fullpath;
	
    playlist->playing = next;

    engine_jump_to(current);

	/* GUI stuff */
	gui_update_playlist();
	gui_update_info();
	gui_update_playback();

	controller_update_whereplaying();
	controller_update_statefile();
}

void controller_play_pause( void )
{
  if(engine_is_paused()){
    playlist->playing->flags &= ~F_PAUSED;
    engine_resume_playback();
  }else if(engine_is_playing()){
    playlist->playing->flags |= F_PAUSED;
    engine_pause_playback();
  }else{
    if ( !playlist->selected ){
      playlist->selected = songdata_next_valid ( playlist, playlist->top, KEY_DOWN );
    }
    controller_jump_to_song ( playlist->selected ); // Play
  }
  /* GUI stuff */
  gui_update_playlist();
  gui_update_info();
  gui_update_playback();
}

void controller_stop( void )
{
	if ( playlist->playing )
	{
		playlist->playing->flags &= ~F_PAUSED;
		playlist->playing = NULL;
		gui_update_playlist();
	}
	engine_stop();
	gui_update_playback();
	controller_update_statefile();
}
void controller_clear_playlist( void )
{
  if (gui_ask_yes_no(CLEARPLAYLIST))
  {
    controller_stop();

    songdata_clear ( playlist );

    gui_update_playlist();
    
    gui_update_info();
    
    update_panels ();
  }
}
void controller_shuffle_playlist( void ){
  if (gui_ask_yes_no(SHUFFLE))
  {
    songdata_randomize(playlist);
    gui_update_playlist();
    gui_update_info();
  }
}
void controller_exit(){
  if (gui_ask_yes_no(EXITPROGRAM))
  {
    bailout ( 0 );
  }
}

void controller_reload_search_results( void ){
  songdata_reload_search_results();
}

void add_to_playlist_recursive ( songdata *list, songdata_song *position, songdata_song *file )
{
	char *prevpwd = NULL;
	songdata *templist = NULL;
	if ( ! ( file->flags & F_DIR ) )
		return;

	templist = malloc( sizeof ( songdata ) );
    memset(templist, 0, sizeof(songdata));
	prevpwd = getcwd ( NULL, 0 );

	songdata_read_mp3_list ( templist, file->fullpath, L_NEW );
	if (templist->selected && !strncmp ( templist->selected->filename, "../", 3 ) )
		templist->selected = templist->head->next; // skip ../ entry

	while ( templist->selected )
	{
		if ( templist->selected->flags & F_DIR )
			add_to_playlist_recursive ( list, list->tail, templist->selected );
		else if ( ! ( templist->selected->flags & F_PLAYLIST ) )
			add_to_playlist ( list, list->tail, templist->selected );

		templist->selected = songdata_next_valid ( templist, templist->selected->next, KEY_DOWN );
	}

	songdata_clear ( templist );
	free ( templist );
	if(chdir ( prevpwd ) != 0){
		//TODO Log this error
	}
	free ( prevpwd );
}

void add_to_playlist ( songdata *list, songdata_song *position, songdata_song *file )
{
	songdata_song *newfile;
	char *p;

	if ( !songdata_check_file ( file ) )
		return;
    	newfile = new_songdata_song();
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

	newfile->track_id = file->track_id;
	newfile->length = file->length;

	songdata_add ( list, position, newfile );

	if ( conf->c_flags & C_PADVANCE )
	{
		list->selected = newfile;
		list->where = list->length;
	}
	return;
}

void controller_init( void )
{
	playlist = malloc ( sizeof ( songdata ) );
    memset(playlist, 0, sizeof(songdata));
	playlist->head = NULL;
	songdata_clear ( playlist );
    http_controller_init(conf);

    if (conf->logfile)
    	logfile = fopen(conf->logfile, "a");

	return;
}

void controller_shutdown ( void )
{
	http_controller_shutdown();
	songdata_clear(playlist);
	free ( playlist );

	if (logfile)
		fclose(logfile);
}


void controller_search(char * string)
{
	pid_t childpid;
    gui_progress_start(SEARCHING);
	handler.sa_handler = SIG_DFL;
	handler.sa_flags = SA_RESETHAND;
	sigaction ( SIGCHLD, &handler, NULL );
	errno = 0;
	if ( ! ( childpid = fork () ) ){
		errno = 0;
        execlp ( "mjsfind", "mjsfind", string, conf->resultsfile, ( char * ) NULL );
		exit ( 3 );
	}
	if ( errno )
		exit ( 3 );
	waitpid ( childpid, NULL, 0 );

    controller_reload_search_results();
}

void controller_save_playlist(char * file)
{
  char *s = malloc ( strlen ( file ) + strlen(conf->playlistpath) + 6 );
  sprintf ( s, "%s/%s.mjs", conf->playlistpath, file );
  songdata_save_playlist ( playlist, s );
  free ( s );
  doupdate ();
}


void controller_playlist_move_up( void )
{
	songdata_song *f1,*f2,*f3,*f4;

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

void controller_playlist_move_down( void )
{
	songdata_song *f1,*f2,*f3,*f4;

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


