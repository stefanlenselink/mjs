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

static struct sigaction handler;

static void process_return (wlist *, int);
static int
    do_save (Input *);
static int
    do_search (Input *);

void
play_next_song(wlist *list)
{
	flist *ftmp = list->playing;

	if (!ftmp)
		return;

	if ((conf->c_flags & C_LOOP) && !ftmp->next)
		ftmp = list->head;
	else
		ftmp = ftmp->next;
	if (!jump_to_song(list, ftmp)) 
		stop_player(list);
}

void
play_prev_song(wlist *list)
{
	flist *ftmp = list->playing;

	if (!ftmp)
		return;
	if ((conf->c_flags & C_LOOP) && !ftmp->prev)
		ftmp = list->tail;
	else
		ftmp = ftmp->prev;
	if (!jump_to_song(list, ftmp)) 
		stop_player(list);
}


int
jump_to_song(wlist *list, flist *next)
{
	char buf[BIG_BUFFER_SIZE+1];
	FILE *activefile;
	flist *ftmp;
    fprintf (stderr, "\n\nmjs:jump to song!\n\n\n");
//	next = next_valid(list, next, KEY_DOWN);
// it is in the playlist so it should be valid anyway
	
	if (!next)
		return 0;
	
/*TODO LastFM    if (list->playing!=NULL && list->playing->has_id3) {
      fprintf(stderr, "Submitting!\n");
      submitLastFM(lastFMHandshake, list->playing->artist, list->playing->title, list->playing->album, list->playing->length, list->playing->track_id);
    }*/
    if(next != NULL && next->has_id3)
    {
      fprintf(stderr, "current!\n");
/* TODO LastFM      currentPlayingLastFM(lastFMHandshake, next->artist, next->title, next->album, next->length, next->track_id);*/
    }else{
      fprintf(stderr, "Whooo no id3! %s %s %s %d %d\n",next->artist, next->title, next->album, next->length, next->track_id);
    }
	list->playing = next;
	memset(buf, 0, sizeof(buf));
	snprintf(buf, BIG_BUFFER_SIZE, "%s", list->playing->fullpath);
    
	engine_play();

	clear_play_info();
	
	
/*	play->update(play);*/
/*	info->update(info);*/
/*	update_title(playback);*/ //TODO update Title of the playback window
	doupdate();

	for (ftmp = list->head, list->whereplaying=0; ftmp!=list->playing; ftmp=ftmp->next)
		list->whereplaying++;

	activefile = fopen(conf->statefile,"w");
	if (activefile) {
		fprintf(activefile,"         Now playing:  %s  (by)  %s  (from)  %s    \n", list->playing->filename, list->playing->artist, list->playing->album);
		fclose(activefile);
	}
	return 1;
}

void
stop_player(wlist *list)
{
	FILE *activefile;
		
	if (list->playing) {
		list->playing->flags &= ~F_PAUSED;
		list->playing = NULL;
//		play->update(play); //TODO anders oplossen
	}


	engine_stop();

/*	update_title(playback);*/ //TODO update the title of hte playback window
	clear_play_info();
	
	activefile = fopen(conf->statefile,"w");
	if (activefile) {
		fprintf(activefile,"%s","                      \n");
		fclose(activefile);
	}

	return;
}

void
pause_player(wlist *list)
{
	FILE *activefile;
	list->playing->flags |= F_PAUSED;
/*  play->update(play);*/ //TODO anders oplossen
	engine_play();
	activefile = fopen(conf->statefile,"w");
	if (activefile) {
		fprintf(activefile,"         Now playing:  %s  (by)  %s  (from)  %s    \n", list->playing->filename, list->playing->artist, list->playing->album);
		fclose(activefile);
	}

	return;
}

void
resume_player(wlist *list)
{
	FILE *active;
	list->playing->flags &= ~F_PAUSED;
/*	play->update(play);*/ //TODO anders oplossen
	engine_play();
	active = fopen(conf->statefile,"w");
	if (active) {
		fprintf(active,"%s","         * Pause *    \n");
		fclose(active);
	}
	return;
}

wlist *
randomize_list(wlist *list)
{
	int i = list->length, j, k;
	flist *ftmp = NULL, *newlist = NULL, **farray = NULL;
	
	if (i < 2)
		return list;
	if (!(farray = (flist **) calloc(i, sizeof(flist *))))
		return list;
	for (ftmp = list->head, j = 0; ftmp; ftmp = ftmp->next, j++) 
		farray[j] = ftmp;
	k = (int)((float)i--*rand()/(RAND_MAX+1.0));
	newlist = farray[k];
	newlist->prev = NULL;
	farray[k] = NULL;
	list->head = list->top = newlist;
	for (ftmp = NULL; i; i--) {
		k = (int)((float)i*rand()/(RAND_MAX+1.0));
		for (j = 0; j <= k; j++)
			if (farray[j] == NULL)
				k++;
		ftmp = farray[k];
		farray[k] = NULL;
		newlist->next = ftmp;
		if (ftmp) {
			ftmp->prev = newlist;
			newlist = ftmp;
		}
	}
	list->selected = list->head;
	list->where = 1;
	list->wheretop = 0;
	list->tail = newlist;
	newlist->next = NULL;
	free(farray);
	return list;
}


void
add_to_playlist_recursive(wlist *list, flist *position, flist *file)
{
	char *prevpwd = NULL;
	wlist *templist = NULL;
	if (!(file->flags & F_DIR))
		return;

	templist = calloc(1, sizeof(wlist));
	prevpwd = getcwd(NULL, 0);

	read_mp3_list(templist, file->fullpath, L_NEW);
	if (!strncmp(templist->selected->filename, "../", 3))
		templist->selected = templist->head->next; // skip ../ entry
	
	while (templist->selected) {
		if (templist->selected->flags & F_DIR)
			add_to_playlist_recursive(list, list->tail, templist->selected);
		else if (!(templist->selected->flags & F_PLAYLIST))
			add_to_playlist(list, list->tail, templist->selected);
		
		templist->selected = next_valid(templist, templist->selected->next, KEY_DOWN);
	}

	wlist_clear(templist);
	free(templist);
	chdir(prevpwd);
	free(prevpwd);
}

void
add_to_playlist(wlist *list, flist *position, flist *file)
{
	flist *newfile;
	char *p;
	
	if (!check_file (file))
		return;
	newfile = calloc(1, sizeof(flist));
	
	/* remove tracknumber if it exists and user wants it*/
	if (!(conf->c_flags & C_TRACK_NUMBERS)) {
		if ((file->filename[0]>='0') & (file->filename[0]<='9')) {
			if ((p = strchr(file->filename, ' ')))
				newfile->filename = strdup(p + 1);
		} else if (!strncasecmp(file->filename,"cd",2))
			newfile->filename = strdup(file->filename+7);	
	}

	if (!newfile->filename)
		newfile->filename = strdup(file->filename);
	if (strlen(newfile->filename) == 0) {
		free(newfile->filename);
		newfile->filename = strdup("...");
	}
		
	newfile->path = strdup(file->path);
	
	newfile->fullpath = strdup(file->fullpath);
	
	if (file->genre)
		newfile->genre = strdup(file->genre);

	if (file->album)
		newfile->album = strdup(file->album);
	
	if (file->artist)
		newfile->artist = strdup(file->artist);

	if (file->title)
		newfile->title = strdup(file->title);
    
    newfile->has_id3 = file->has_id3;
    newfile->track_id = file->track_id;
    newfile->length = file->length;
	
	wlist_add(list, position, newfile); 

	if (conf->c_flags & C_PADVANCE) {
		list->selected = newfile;
		list->where = list->length;
	} 
	return;
}

void controller_init(Config * init_config)
{
  conf = init_config;
  previous_selected = strdup ("\0");
}

void controller_shutdown(void)
{
  //No function jet
}

static void process_return (wlist * fileslist, int alt)
{

  wlist *playlist = NULL; //play->contents.list; //TODO anders oplossen
  if (!fileslist)
    return;



  if ((fileslist->selected->flags & F_DIR)) {
    if (!alt) {
			// change to another directory

      if (!strcmp ("../", fileslist->selected->filename)) {
        char * filename = strdup(dirstack_filename());
        char * fullpath = strdup(dirstack_fullpath());
        dirstack_pop();
        read_mp3_list (fileslist, fullpath, L_NEW);
        while (strcmp (fileslist->selected->filename, filename))
          move_files_selector(KEY_DOWN);
        free(filename);
        free(fullpath);
      } else {
        if (!(fileslist->flags & F_VIRTUAL)) 
          dirstack_push(fileslist->from, fileslist->selected->filename);
        read_mp3_list (fileslist, fileslist->selected->fullpath, L_NEW);
      }

/*      files->update (files);*/ //TODO anders oplossen
    } else {
			// add songs from directory
      if (previous_selected)
        free (previous_selected);
      previous_selected = strdup (fileslist->selected->fullpath);
      if ((!(fileslist->flags & F_VIRTUAL)) & (strcmp ("../", fileslist->selected->fullpath))) {
        add_to_playlist_recursive (playlist, playlist->tail, fileslist->selected);
        //play->update(play); //TODO anders oplossen
      }
    }


  } else if (fileslist->selected->flags & F_PLAYLIST) {
//		if ((alt > 0) ^ ((conf->c_flags & C_P_TO_F) > 0))	// load playlist directly with alt-enter
    if (!alt)
    {
      dirstack_push(fileslist->from, fileslist->selected->filename);
      read_mp3_list (fileslist, fileslist->selected->fullpath, L_NEW);
/*      files->update (files);*/ //TODO anders oplossen
    } else {
//			read_mp3_list (playlist, fileslist->selected->fullpath, L_APPEND);
      add_to_playlist_recursive (playlist, playlist->tail, fileslist->selected);
      /*play->update (play);*/ //TODO anders oplossen
    }

    update_panels ();


  } else			// normal mp3
    if (strcmp (previous_selected, fileslist->selected->fullpath)) {	// we dont want to add the last file multiple times 
      if (previous_selected)
        free (previous_selected);
      previous_selected = strdup (fileslist->selected->fullpath);

      if (!alt)
        add_to_playlist (playlist, playlist->tail, fileslist->selected);
      else
        add_to_playlist (playlist, playlist->selected, fileslist->selected);
 /*     play->update(play);*/
      if (conf->c_flags & C_FADVANCE)
        if (move_files_selector(KEY_DOWN)) {
/*       info->update (info);*/
/*        files->update (files);*/ //TODO anders oplossen
        }
    }

}


int read_keyboard (Window * window)
{
  int c, alt = 0;
  Input *inputline = window->inputline;
  wlist *mp3list =  NULL; //files->contents.list; TODO anders oplossen

  c = wgetch (window->win);
  if (c == 27) {
    alt = 1;
    c = wgetch (window->win);
  }

  if (inputline)
    return inputline->parse (inputline, c, alt);

  switch (c) {

    case '\t':
		// Switch between files and playlist window
//      if (active->next)
  //      change_active (active->next); //TODO anders oplossen
      break;

    case KEY_BTAB:
		// Switch between files and playlist window
//      if (active->prev)
  //      change_active (active->prev); //TODO anders oplossen
      break;

    case '-':
		// Move selected forward in playlist                    
      if (1/*(*active == play) && !(play->contents.list->selected == play->contents.list->head)*/) {
        //move_backward (play->contents.list);
        //play->update (play);
        //TODO anders oplossen
      }
      break;

    case '+':
    case '=':
		// Move selected backwards in playlist                  
      if (1/*(active == play) && !(play->contents.list->selected == play->contents.list->tail)*/) {
        //move_forward (play->contents.list);
        //play->update (play);
        //TODO anders oplossen
      }
      break;


    case KEY_DOWN:
    case KEY_UP:
    case KEY_HOME:
    case KEY_END:
    case KEY_PPAGE:
    case KEY_NPAGE:
      if ((window->flags & W_LIST)
           && (move_selector (window, c))) {
        window->update (window);
/*        info->update(info);*/
           }
           break;


    case KEY_ENTER:
    case '\n':
    case '\r':
		// File selection / directory navigation                
      if (1 /*active == files*/)
        process_return (window->contents.list, alt);
      else {
        move_selector (window, c);
//        play->update(play);//TODO anders oplossen
      }
      break;

    case KEY_IC:
      if (!((mp3list->selected->flags & F_DIR) | (mp3list->selected->flags & F_PLAYLIST)))
        if (strcmp (previous_selected, mp3list->selected->fullpath)) {	// we dont want to add the last file multiple times 
        if (previous_selected)
          free (previous_selected);
        previous_selected = strdup (mp3list->selected->fullpath);

      //  if (1/*play->contents.list->playing*/) //TODO anders oplossen
//          add_to_playlist (play->contents.list, play->contents.list->playing, mp3list->selected);
    //    else
  //        add_to_playlist (play->contents.list, play->contents.list->selected, mp3list->selected);
        if (conf->c_flags & C_FADVANCE)
          if (move_files_selector (KEY_DOWN)) {
/*          files->update (files);*/ //TODO anders oplossen
//          info->update (info);
          }
  //        play->update (play);
        }
        break;

    case KEY_LEFT:
		// leave directory
      if ((1/*active == files*/) && !dirstack_empty()) { //TODO anders oplossen
        char * filename = strdup(dirstack_filename());
        char * fullpath = strdup(dirstack_fullpath());
        dirstack_pop();
        read_mp3_list (mp3list, fullpath, L_NEW);
        while (strcmp (mp3list->selected->filename, filename))
          move_files_selector (KEY_DOWN);
/*       files->update (files);*/ //TODO anders oplossen
//        info->update (info);
        free(filename);
        free(fullpath);
      }
      break;

    case KEY_RIGHT:
// enter directory
      if (1 /*active == files*/) { //TODO anders oplossen
        if ((mp3list->selected->flags & (F_DIR|F_PLAYLIST))
             && (strncmp(mp3list->selected->filename, "../",3))) {
          if (!(mp3list->flags & F_VIRTUAL)) 
            dirstack_push(mp3list->from, mp3list->selected->filename);
          read_mp3_list (mp3list, mp3list->selected->fullpath, L_NEW);
          if (mp3list->head) 
            move_files_selector (KEY_DOWN);
             }
		
/*             files->update (files);*/ //TODO anders oplossen
//             info->update (info);
      }
      break;


    case KEY_DC:
		// remove selected from playlist                
/*      if ((active == play) && (play->contents.list->selected)) {
        wlist *playlist = play->contents.list;
        if ( (play->contents.list->playing == play->contents.list->selected)) {
          play_next_song (play->contents.list);
          play->update (play);
        }
        wlist_del(playlist, playlist->selected);*/ //TODO anders oplossen
//        info->update (info);
//        play->update (play);
      //}
      break;

    case KEY_REFRESH:
    case '~':
    case '`':
		// refresh screen                       
      wrefresh (curscr);
      break;

    case KEY_F (1):
		// Exit mjs                     
/*      menubar->deactivate (menubar); */ //TODO op nieuwe manier aanroepen
      printf_menubar (EXITPROGRAM);
      c = wgetch (window->win);
      if (c == 27)
        c = wgetch (window->win);
      if ((c == 'y') | (c == 'Y'))
        bailout (0);
 /*     menubar->activate (menubar);*/ //TODO op nieuwe manier aanroepen
      update_panels ();
      break;

    case KEY_F (2):
		// Clear playlist
/*      menubar->deactivate (menubar);*/ //TODO op nieuwe manier aanroepen
      printf_menubar (CLEARPLAYLIST);
      c = wgetch (window->win);
      if (c == 27)
        c = wgetch (window->win);
      if ((c == 'y') | (c == 'Y')) {
//        stop_player (play->contents.list);
        clear_play_info ();

 //       wlist_clear(play->contents.list); //TODO anders oplossen

//        play->update (play);
        clear_info ();
//        info->update (info);
      }
/*      menubar->activate (menubar);*/ //TODO op nieuwe manier aanroepen
      update_panels ();
      break;

    case KEY_F (3):
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

    case KEY_F (4):
		// Show last search results
      if (!(mp3list->flags & F_VIRTUAL))
        dirstack_push(mp3list->from, mp3list->selected->filename);
      read_mp3_list (mp3list, conf->resultsfile, L_SEARCH);
//      info->update (info);
/*      files->update (files);*/ //TODO anders oplossen
      break;

    case KEY_F (5):
		// Randomize the playlist                               
/*      menubar->deactivate (menubar);*/ //TODO op nieuwe manier aanroepen
      printf_menubar (SHUFFLE);
      c = wgetch (window->win);
      if (c == 27)
        c = wgetch (window->win);
      if ((c == 'y') | (c == 'Y')) {
//        randomize_list (play->contents.list); //TODO anders oplossen
//        play->update (play);
//        info->update (info);
      }
/*     menubar->activate (menubar);*/ //TODO op nieuwe manier aanroepen
      break;

    case KEY_F (6):
		// Save Playlist
//      if ((!(conf->c_flags & C_ALLOW_P_SAVE)) | (!(play->contents.list->head))) //TODO anders oploseen
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

    case KEY_F (7):
		// Stop the player              
//      stop_player (play->contents.list); //TODO anders lossen
      break;

      case KEY_F (8): /* TODO Big overhull*/
		// Play / Pause key
			// fix me 
/*        if (!play->contents.list->selected)
          play->contents.list->selected = next_valid (play->contents.list, play->contents.list->top, KEY_DOWN);
        jump_to_song (play->contents.list, play->contents.list->selected);	// Play 
        break;
        pause_player (play->contents.list);	// Pause / Verdergaan 
        break;
        resume_player (play->contents.list);	// Pause / Verdergaan */ //TODO anders lopssen
        break;
        break;

    case KEY_F (9):
		// Skip to previous mp3 in playlist     
//      play_prev_song (play->contents.list); //TODO anders oplossen
		
      break;

    case KEY_F (10):
		// Skip JUMP frames backward

      engine_frwd(conf->jump);
      break;

    case KEY_F (11):

      engine_ffwd(conf->jump);
      break;

    case KEY_F (12):
		// Skip to next mp3 in playlist                 

//      play_next_song (play->contents.list); //TODO anders
		
      break;

    case 'a'...'z':
    case 'A'...'Z':
    case '0'...'9':
    case '.':
    case ' ':
		// Jump to directory with matching first letters
      if (1/*active == files*/){ //TODO anders oplossen
        flist *ftmp = mp3list->head;
        int n = 0;
        if (strlen (typed_letters) < 10) { // add the letter to the string and reset the timeout
          strcat (typed_letters, (char *) &c);
          typed_letters_timeout = 4;
        }
			
        while (strncasecmp (ftmp->filename, (char *) &typed_letters, strlen (typed_letters))) {
          if (ftmp == mp3list->tail) { // end of the list reached without result
            ftmp = NULL;
            break;
          }
          ftmp = ftmp->next;
          n++;
        }

        if (ftmp) { // match found
          mp3list->selected = ftmp;
          mp3list->where = n;
/*          files->update(files);*/ //TODO anders oplossen
        }
      }
      break;
		
    case KEY_BACKSPACE:
      if (1 /*active == files*/) { //TODO anders oplossen
        if (strlen(typed_letters) > 1) {
          typed_letters[strlen(typed_letters) - 1] = '\0';
          typed_letters_timeout = 4;
        } else 	if (strlen(typed_letters) == 1) {
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
    do_save (Input * input)
{
  char *s = malloc (strlen (input->buf) + 26);
//  active = old_active;
  sprintf (s, "%s/%s.mjs", conf->playlistpath, input->buf);
//  write_mp3_list_file (play->contents.list, s); //TODO anders
  free (s);
  free (input);
/*  menubar->activate (menubar);*/ //TODO op nieuwe manier aanroepen
/*  menubar->inputline = NULL;*/ //TODO op nieuwe manier aanroepen
  doupdate ();
  return 1;
}

static int
    do_search (Input * input)
{
  pid_t childpid;
  wlist *mp3list = NULL; //files->contents.list; //TODO anders oplossen
//  active = old_active;
  if (!((*input->buf == ' ') || (*input->buf == '\0'))) {
    handler.sa_handler = SIG_DFL;
    handler.sa_flags = SA_ONESHOT;
    sigaction (SIGCHLD, &handler, NULL);
    errno = 0;
    if (!(childpid = fork ())) {
      errno = 0;
      execlp ("findmp3", "findmp3", input->buf, conf->resultsfile, (char *) NULL);
      exit (3);
    }
    if (errno)
      exit (3);

    waitpid (childpid, NULL, 0);

    handler.sa_handler = (SIGHANDLER) unsuspend;
    handler.sa_flags = SA_RESTART;
    sigaction (SIGCONT, &handler, NULL);
/*
    obsolete by new engine system?
        
    handler.sa_handler = (SIGHANDLER) restart_mpg_child;
    handler.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction (SIGCHLD, &handler, NULL);*/
    if (!(mp3list->flags & F_VIRTUAL))
      dirstack_push(mp3list->from, mp3list->selected->filename);
    read_mp3_list (mp3list, conf->resultsfile, L_SEARCH);
/*    files->update (files);*/ //TODO anders oplossen
//    info->update(info);
  } else
/*    menubar->activate (menubar);
	
    free (input);
    menubar->inputline = NULL;
    files->update (files); *///TODO op nieuwe manier aanroepen
    doupdate ();
    return 1;
}
