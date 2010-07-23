#include "keyboard_controller.h"
#include "engine/engine.h"
#include "controller/controller.h"
#include "gui/window_play.h"
#include "gui/window_info.h"
#include "gui/window_files.h"

#include <string.h>
#include <pthread.h>
char *previous_selected;	// previous selected number
char typed_letters[11] = "\0";	// letters previously typed when jumping
int typed_letters_timeout = 0;	// timeout for previously typed letters

songdata * playlist;
Config * conf;
pthread_t keyboard_thread;

/* Private functions */
static void process_return ( songdata *, int );

static void process_return ( songdata * fileslist, int alt )
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
        songdata_read_mp3_list ( fileslist, fullpath, L_NEW );
        while ( strcmp ( fileslist->selected->filename, filename ) )
          move_files_selector ( KEY_DOWN );
        free ( filename );
        free ( fullpath );
      }
      else
      {
        if ( ! ( fileslist->flags & F_VIRTUAL ) )
          dirstack_push ( fileslist->from, fileslist->selected->filename );
        songdata_read_mp3_list ( fileslist, fileslist->selected->fullpath, L_NEW );
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
        doupdate();
      }
    }


  }
  else if ( fileslist->selected->flags & F_PLAYLIST )
  {
//		if ((alt > 0) ^ ((conf->c_flags & C_P_TO_F) > 0))	// load playlist directly with alt-enter
    if ( !alt )
    {
      dirstack_push ( fileslist->from, fileslist->selected->filename );
      songdata_read_mp3_list ( fileslist, fileslist->selected->fullpath, L_NEW );
      window_files_update();
    }
    else
    {
//			songdata_read_mp3_list (playlist, fileslist->selected->fullpath, L_APPEND);
      add_to_playlist_recursive ( playlist, playlist->tail, fileslist->selected );
      window_play_update();
      doupdate();
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

      update_panels();
      doupdate();
}


void keyboard_controller_init(songdata * init_playlist, Config * init_conf)
{
  playlist = init_playlist;
  conf = init_conf;
  previous_selected = strdup ( "\0" );

  //Start keyboard thread
  pthread_create(&keyboard_thread, NULL, keyboard_controller_thread, NULL);
}


int keyboard_controller_read_key(Window * window)
{
  int c, alt = 0;
  c = wgetch ( window->win );
  if(c == ERR){
    //Nothing happend
    return c;
  }
  if ( c == 27 )
  {
    alt = 1; //Alt detected get the next character
    c = wgetch ( window->win );
  }

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
      if ( ! ( ( window->contents.list->selected->flags & F_DIR ) | ( window->contents.list->selected->flags & F_PLAYLIST ) ) )
        if ( strcmp ( previous_selected, window->contents.list->selected->fullpath ) )  	// we dont want to add the last file multiple times
      {
        if ( previous_selected )
          free ( previous_selected );
        previous_selected = strdup ( window->contents.list->selected->fullpath );

        if ( playlist->playing )
          add_to_playlist ( playlist, playlist->playing, window->contents.list->selected );
        else
          add_to_playlist ( playlist, playlist->selected, window->contents.list->selected );
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
        songdata_read_mp3_list ( window->contents.list, fullpath, L_NEW );
        while ( strcmp ( window->contents.list->selected->filename, filename ) )
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
        if ( ( window->contents.list->selected->flags & ( F_DIR|F_PLAYLIST ) )
               && ( strncmp ( window->contents.list->selected->filename, "../",3 ) ) )
        {
          if ( ! ( window->contents.list->flags & F_VIRTUAL ) )
            dirstack_push ( window->contents.list->from, window->contents.list->selected->filename );
          songdata_read_mp3_list ( window->contents.list, window->contents.list->selected->fullpath, L_NEW );
          if ( window->contents.list->head )
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
        if (playlist->playing == playlist->selected && playlist->playing->next != NULL)
        {
          controller_next( playlist );
          window_play_update();
        }else if (playlist->playing == playlist->selected && playlist->playing->next == NULL){
          controller_stop();
          window_play_update();
        }
        songdata_del ( playlist, playlist->selected );
        window_info_update();
        window_play_update();
      }
      break;

    case KEY_REFRESH:
    case '~':
    case '`': // refresh screen
      wrefresh ( curscr );
      break;

    case KEY_F ( 1 ) : // Exit mjs
      controller_exit();
      break;

    case KEY_F ( 2 ) : // Clear playlist
      controller_clear_playlist();
      break;
  
    case KEY_F ( 3 ) :
      {
        char buf[512];
        memset(&buf, 0, 512);
        if(gui_ask_question("Search for: ", buf)){
          //there is input
          controller_search(buf);
        }
      }
      break;

    case KEY_F ( 4 ) : // Show last search results
      controller_reload_search_results();
      break;

    case KEY_F ( 5 ) : // Randomize the playlist
      controller_shuffle_playlist();
      break;

    case KEY_F ( 6 ) :
						// Save Playlist
      if ( ( ! ( conf->c_flags & C_ALLOW_P_SAVE ) ) | ( ! ( playlist->head ) ) )
        break;
      {
        char buf[512];
        if(gui_ask_question("Save as: ", buf)){
          //there is input
          controller_save_playlist(buf);
        }
      }
      break;

    case KEY_F ( 7 ) : // Stop the player
      controller_stop();
      break;
    case KEY_F ( 8 ) : // Play / Pause the player
        controller_play_pause();
        break;
    case KEY_F ( 9 ) : // Skip to previous mp3 in playlist
      controller_prev();
      break;
    case KEY_F ( 10 ) : // FRWD
      engine_frwd ( conf->jump , conf->jumpExpFactor);
      break;
    case KEY_F ( 11 ) : //FFWD
      engine_ffwd ( conf->jump , conf->jumpExpFactor);
      break;
    case KEY_F ( 12 ) : // Skip to next mp3 in playlist
      controller_next( playlist );
      break;
    case 'a'...'z':
    case 'A'...'Z':
    case '0'...'9':
    case '.':
    case ' ': // Jump to directory with matching first letters
      if ( window->name == window_files){
        songdata_song *ftmp = window->contents.list->head;
        int n = 0;
        if ( strlen ( typed_letters ) < 10 )   // add the letter to the string and reset the timeout
        {
          strcat ( typed_letters, ( char * ) &c );
          typed_letters_timeout = 10;
        }

        while ( strncasecmp ( ftmp->filename, ( char * ) &typed_letters, strlen ( typed_letters ) ) )
        {
          if ( ftmp == window->contents.list->tail ) // end of the list reached without result
          {
            ftmp = NULL;
            break;
          }
          ftmp = ftmp->next;
          n++;
        }

        if ( ftmp ) // match found
        {
          window->contents.list->selected = ftmp;
          window->contents.list->where = n;
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
          typed_letters_timeout = 10;
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
  return c;
}

void keyboard_controller_shutdown(void)
{
  //Nog niets
}

void keyboard_controller_check_timeout()
{
  if ( typed_letters_timeout >= 0 )
  {
    if ( typed_letters_timeout == 0 )
      typed_letters[0] = '\0';
    typed_letters_timeout--;
  }
}

void * keyboard_controller_thread(void * args){
	while(1){
		poll_keyboard();
	}

}
