#include "keyboard_controller.h"

#include <string.h>

char *previous_selected;	// previous selected number
char typed_letters[10] = "\0";	// letters previously typed when jumping
int typed_letters_timeout = 0;	// timeout for previously typed letters

wlist * playlist;
Config * conf;
/* Private functions */
static void process_return ( wlist *, int );

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


void keyboard_controller_init(wlist * init_playlist, Config * init_conf)
{
  playlist = init_playlist;
  conf = init_conf;
  previous_selected = strdup ( "\0" );
}


int keyboard_controller_read_key(Window * window)
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
        read_mp3_list ( window->contents.list, fullpath, L_NEW );
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
          read_mp3_list ( window->contents.list, window->contents.list->selected->fullpath, L_NEW );
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
        controller_stop();

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
      if ( ! ( window->contents.list->flags & F_VIRTUAL ) )
        dirstack_push ( window->contents.list->from, window->contents.list->selected->filename );
      read_mp3_list ( window->contents.list, conf->resultsfile, L_SEARCH );
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
        songdata_randomize(playlist);
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
      controller_stop();
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
      controller_prev();

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

      controller_next( playlist );

      break;

    case 'a'...'z':
    case 'A'...'Z':
    case '0'...'9':
    case '.':
    case ' ':

      printf ( "Key\n" );
			// Jump to directory with matching first letters
      if ( window->name == window_files){
        flist *ftmp = window->contents.list->head;
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
  doupdate ();
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
