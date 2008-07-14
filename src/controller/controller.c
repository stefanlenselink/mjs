#include "defs.h"
#include "controller.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "gui/gui.h"
#include "gui/inputline.h"
#include "mjs.h"

#include <string.h>


Config * conf;
Window * old_active;
char *previous_selected;	// previous selected number
char typed_letters[10] = "\0";	// letters previously typed when jumping
int typed_letters_timeout = 0;	// timeout for previously typed letters

wlist * playlist, * mp3list;

static struct sigaction handler;

static void process_return ( wlist *, int );
static int
do_save ( Input * );
static int
do_search ( Input * );

void
play_next_song()
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

void
play_prev_song()
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

void controller_update_whereplaying ( void )
{
	flist *ftmp;
	for ( ftmp = playlist->head, playlist->whereplaying=0; ftmp!=playlist->playing; ftmp=ftmp->next )
		playlist->whereplaying++;
}

void controller_update_statefile ( void )
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

void
stop_player()
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
	return;
}


wlist *
randomize_list ( wlist *list )
{
	int i = list->length, j, k;
	flist *ftmp = NULL, *newlist = NULL, **farray = NULL;

	if ( i < 2 )
		return list;
	if ( ! ( farray = ( flist ** ) calloc ( i, sizeof ( flist * ) ) ) )
		return list;
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
	return list;
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

wlist * controller_init ( Config * init_config, wlist * init_mp3list )
{
	conf = init_config;
	playlist = malloc ( sizeof ( wlist ) );
	playlist->head = NULL;
	wlist_clear ( playlist );
	mp3list = init_mp3list;
	previous_selected = strdup ( "\0" );
	return playlist;
}

void controller_shutdown ( void )
{
	free ( playlist );
}

static void process_return ( wlist * fileslist, int alt )
{
	if ( !fileslist )
		return;



	if ( ( fileslist->selected->flags & F_DIR ) )
	{
		if ( !alt )
		{
			// change to another directory

			if ( !strcmp ( "../", fileslist->selected->filename ) )
			{
				char * filename = strdup ( dirstack_filename() );
				char * fullpath = strdup ( dirstack_fullpath() );
				dirstack_pop();
				read_mp3_list ( fileslist, fullpath, L_NEW );
				while ( strcmp ( fileslist->selected->filename, filename ) )
					move_files_selector ( KEY_DOWN );
				free ( filename );
				free ( fullpath );
			}
			else
			{
				if ( ! ( fileslist->flags & F_VIRTUAL ) )
					dirstack_push ( fileslist->from, fileslist->selected->filename );
				read_mp3_list ( fileslist, fileslist->selected->fullpath, L_NEW );
			}

			window_files_update();
		}
		else
		{
			// add songs from directory
			if ( previous_selected )
				free ( previous_selected );
			previous_selected = strdup ( fileslist->selected->fullpath );
			if ( ( ! ( fileslist->flags & F_VIRTUAL ) ) & ( strcmp ( "../", fileslist->selected->fullpath ) ) )
			{
				add_to_playlist_recursive ( playlist, playlist->tail, fileslist->selected );
				window_play_update();
			}
		}


	}
	else if ( fileslist->selected->flags & F_PLAYLIST )
	{
//		if ((alt > 0) ^ ((conf->c_flags & C_P_TO_F) > 0))	// load playlist directly with alt-enter
		if ( !alt )
		{
			dirstack_push ( fileslist->from, fileslist->selected->filename );
			read_mp3_list ( fileslist, fileslist->selected->fullpath, L_NEW );
			window_files_update();
		}
		else
		{
//			read_mp3_list (playlist, fileslist->selected->fullpath, L_APPEND);
			add_to_playlist_recursive ( playlist, playlist->tail, fileslist->selected );
			window_play_update();
		}

		update_panels ();


	}
	else			// normal mp3
		if ( strcmp ( previous_selected, fileslist->selected->fullpath ) )  	// we dont want to add the last file multiple times
		{
			if ( previous_selected )
				free ( previous_selected );
			previous_selected = strdup ( fileslist->selected->fullpath );

			if ( !alt )
				add_to_playlist ( playlist, playlist->tail, fileslist->selected );
			else
				add_to_playlist ( playlist, playlist->selected, fileslist->selected );
			window_play_update();
			if ( conf->c_flags & C_FADVANCE )
				if ( move_files_selector ( KEY_DOWN ) )
				{
					window_info_update();
					window_files_update();
				}
		}

}


int read_keyboard ( Window * window )
{
	int c, alt = 0;
	Input *inputline = window->inputline;

	c = wgetch ( window->win );
	if ( c == 27 )
	{
		alt = 1;
		c = wgetch ( window->win );
	}

	if ( inputline )
		return inputline->parse ( inputline, c, alt );

	switch ( c )
	{

		case '\t':
			// Switch between files and playlist window
			change_active ( 1 );
			break;

		case KEY_BTAB:
			change_active ( 0 );
			break;

		case '-':
			// Move selected forward in playlist
			if ( ( window->name  == window_play ) && ! ( playlist->selected == playlist->head ) )
			{
				controller_playlist_move_up ();
				window_play_update();
			}
			break;

		case '+':
		case '=':
			// Move selected backwards in playlist
			if ( ( window->name == window_play ) && ! ( playlist->selected == playlist->tail ) )
			{
				controller_playlist_move_down ();
				window_play_update();
			}
			break;


		case KEY_DOWN:
		case KEY_UP:
		case KEY_HOME:
		case KEY_END:
		case KEY_PPAGE:
		case KEY_NPAGE:
			if ( ( window->flags & W_LIST )
			        && ( move_selector ( window, c ) ) )
			{
				window->update ( window );
				window_info_update();
			}
			break;


		case KEY_ENTER:
		case '\n':
		case '\r':
			// File selection / directory navigation
			if ( window->name == window_files )
				process_return ( window->contents.list, alt );
			else
			{
				move_selector ( window, c );
				window_play_update();
			}
			break;

		case KEY_IC:
			if ( ! ( ( mp3list->selected->flags & F_DIR ) | ( mp3list->selected->flags & F_PLAYLIST ) ) )
				if ( strcmp ( previous_selected, mp3list->selected->fullpath ) )  	// we dont want to add the last file multiple times
				{
					if ( previous_selected )
						free ( previous_selected );
					previous_selected = strdup ( mp3list->selected->fullpath );

					if ( playlist->playing )
						add_to_playlist ( playlist, playlist->playing, mp3list->selected );
					else
						add_to_playlist ( playlist, playlist->selected, mp3list->selected );
					if ( conf->c_flags & C_FADVANCE )
						if ( move_files_selector ( KEY_DOWN ) )
						{
							window_files_update();
							window_info_update();
						}
					window_play_update();
				}
			break;

		case KEY_LEFT:
			// leave directory
			if ( ( window->name == window_files ) && !dirstack_empty() )
			{
				char * filename = strdup ( dirstack_filename() );
				char * fullpath = strdup ( dirstack_fullpath() );
				dirstack_pop();
				read_mp3_list ( mp3list, fullpath, L_NEW );
				while ( strcmp ( mp3list->selected->filename, filename ) )
					move_files_selector ( KEY_DOWN );
				window_files_update();
				window_info_update();
				free ( filename );
				free ( fullpath );
			}
			break;

		case KEY_RIGHT:
// enter directory
			if ( window->name == window_files )
			{
				if ( ( mp3list->selected->flags & ( F_DIR|F_PLAYLIST ) )
				        && ( strncmp ( mp3list->selected->filename, "../",3 ) ) )
				{
					if ( ! ( mp3list->flags & F_VIRTUAL ) )
						dirstack_push ( mp3list->from, mp3list->selected->filename );
					read_mp3_list ( mp3list, mp3list->selected->fullpath, L_NEW );
					if ( mp3list->head )
						move_files_selector ( KEY_DOWN );
				}

				window_files_update();
				window_info_update();
			}
			break;


		case KEY_DC:
			// remove selected from playlist
			if ( ( window->name == window_play ) && ( playlist->selected ) )
			{
				if ( ( playlist->playing == playlist->selected ) )
				{
					play_next_song ( playlist );
					window_play_update();
				}
				wlist_del ( playlist, playlist->selected );
				window_info_update();
				window_play_update();
			}
			break;

		case KEY_REFRESH:
		case '~':
		case '`':
			// refresh screen
			wrefresh ( curscr );
			break;

		case KEY_F ( 1 ) :
						// Exit mjs
						window_menubar_deactivate();
			printf_menubar ( EXITPROGRAM );
			c = wgetch ( window->win );
			if ( c == 27 )
				c = wgetch ( window->win );
			if ( ( c == 'y' ) | ( c == 'Y' ) )
				bailout ( 0 );
			window_menubar_activate();
			update_panels ();
			break;

		case KEY_F ( 2 ) :
						// Clear playlist
						window_menubar_deactivate();
			printf_menubar ( CLEARPLAYLIST );
			c = wgetch ( window->win );
			if ( c == 27 )
				c = wgetch ( window->win );
			if ( ( c == 'y' ) | ( c == 'Y' ) )
	{
				stop_player ( playlist );

				wlist_clear ( playlist );

				window_play_update();
				clear_info ();
				window_info_update();
			}
			window_menubar_activate();
			update_panels ();
			break;

		case KEY_F ( 3 ) :
						// Search in mp3-database
//      old_active = active;
						/*      active = menubar;*/ //TODO op nieuwe manier aanroepen
						/*      menubar->inputline = inputline = (Input *) calloc (1, sizeof (Input)); //TODO op nieuwe manier aanroepen
						      inputline->win = menubar->win;
						      inputline->panel = menubar->panel;
						      inputline->x = inputline->y = 0;
						      strncpy (inputline->prompt, "Search for:", 39);
						      inputline->plen = strlen (inputline->prompt);
						      inputline->flen = 70;
						      inputline->anchor = inputline->buf;
						      inputline->parse = do_inputline;
						      inputline->update = update_menu;
						      inputline->finish = do_search;
						      inputline->complete = filename_complete;
						      inputline->pos = 1;
						      inputline->fpos = 1;
						      update_menu (inputline);*/
						break;

		case KEY_F ( 4 ) :
						// Show last search results
						if ( ! ( mp3list->flags & F_VIRTUAL ) )
							dirstack_push ( mp3list->from, mp3list->selected->filename );
			read_mp3_list ( mp3list, conf->resultsfile, L_SEARCH );
			window_info_update();
			window_files_update();
			break;

		case KEY_F ( 5 ) :
						// Randomize the playlist
						window_menubar_deactivate();
			printf_menubar ( SHUFFLE );
			c = wgetch ( window->win );
			if ( c == 27 )
				c = wgetch ( window->win );
			if ( ( c == 'y' ) | ( c == 'Y' ) )
	{
				randomize_list ( playlist );
				window_play_update();
				window_info_update();
			}
			window_menubar_activate();
			break;

		case KEY_F ( 6 ) :
						// Save Playlist
						if ( ( ! ( conf->c_flags & C_ALLOW_P_SAVE ) ) | ( ! ( playlist->head ) ) )
							break;
//      old_active = active;
			/*      active = menubar;
			      clear_menubar (menubar);
			      menubar->inputline = inputline = (Input *) calloc (1, sizeof (Input));
			      inputline->win = menubar->win;
			      inputline->panel = menubar->panel;
			      inputline->x = inputline->y = 0;
			      strncpy (inputline->prompt, "Save as:", 39);
			      inputline->plen = strlen (inputline->prompt);
			      inputline->flen = 70;
			      inputline->anchor = inputline->buf;
			      inputline->parse = do_inputline;
			      inputline->update = update_menu;
			      inputline->finish = do_save;
			      inputline->complete = filename_complete;
			      inputline->pos = 1;
			      inputline->fpos = 1;
			      update_menu (inputline);*/ //TODO op nieuwe manier aanroepen
			break;

		case KEY_F ( 7 ) :
						// Stop the player
						stop_player ( playlist );
			break;

		case KEY_F ( 8 ) : /* TODO Big overhull*/
						// Play / Pause key
						// fix me
						if ( !playlist->selected )
							playlist->selected = next_valid ( playlist, playlist->top, KEY_DOWN );
			controller_jump_to_song ( playlist->selected ); // Play
			break;

		case KEY_F ( 9 ) :
						// Skip to previous mp3 in playlist
						play_prev_song ();

			break;

		case KEY_F ( 10 ) :
						// Skip JUMP frames backward

						engine_frwd ( conf->jump );
			break;

		case KEY_F ( 11 ) :

						engine_ffwd ( conf->jump );
			break;

		case KEY_F ( 12 ) :
						// Skip to next mp3 in playlist

						play_next_song ( playlist );

			break;

		case 'a'...'z':
		case 'A'...'Z':
		case '0'...'9':
		case '.':
		case ' ':
			// Jump to directory with matching first letters
			if ( window->name == window_files )
	{
				flist *ftmp = mp3list->head;
				int n = 0;
				if ( strlen ( typed_letters ) < 10 )   // add the letter to the string and reset the timeout
				{
					strcat ( typed_letters, ( char * ) &c );
					typed_letters_timeout = 4;
				}

				while ( strncasecmp ( ftmp->filename, ( char * ) &typed_letters, strlen ( typed_letters ) ) )
				{
					if ( ftmp == mp3list->tail ) // end of the list reached without result
					{
						ftmp = NULL;
						break;
					}
					ftmp = ftmp->next;
					n++;
				}

				if ( ftmp ) // match found
				{
					mp3list->selected = ftmp;
					mp3list->where = n;
					window_files_update();
				}
			}
			break;

		case KEY_BACKSPACE:
			if ( window->name == window_files )
			{
				if ( strlen ( typed_letters ) > 1 )
				{
					typed_letters[strlen ( typed_letters ) - 1] = '\0';
					typed_letters_timeout = 4;
				}
				else 	if ( strlen ( typed_letters ) == 1 )
				{
					typed_letters[0] = '\0';
					typed_letters_timeout = 0;
				}
			}
			break;

		default:
			break;
	}
	doupdate ();
	return c;
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

		if ( ! ( mp3list->flags & F_VIRTUAL ) )
			dirstack_push ( mp3list->from, mp3list->selected->filename );
		read_mp3_list ( mp3list, conf->resultsfile, L_SEARCH );
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
