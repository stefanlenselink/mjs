#include "gui.h"

#include <string.h>
#include <time.h>

songdata_song *previous_selected;
static char typed_letters[11] = "\0"; // letters previously typed when jumping
static time_t typed_letters_last_activity; // timeout for previously typed letters

void gui_init_filelist(void) {
	filelist_window = window_new();
	filelist_window->list = mp3list;
	filelist_window->type = WINDOW_TYPE_LIST;
	filelist_window->update = gui_update_filelist;
	filelist_window->activate = gui_activate_filelist;
	filelist_window->deactivate = gui_deactivate_filelist;
	filelist_window->input = gui_input_filelist;

	if (conf->files_window.height == 0) {
		conf->files_window.height = LINES - conf->files_window.y;
	}
	if (conf->files_window.width < 4) {
		conf->files_window.width = COLS - conf->files_window.x;
	}
	window_init(filelist_window, WINDOW_NAME_FILELIST, &conf->files_window);

	time(&typed_letters_last_activity);
}

void gui_update_filelist(void) {
	if(!filelist_window){
		return;
	}
	window_draw_border(filelist_window);
	window_draw_title(filelist_window);
	window_draw_scrollbar(filelist_window);
	window_draw_list(filelist_window);
	window_update(filelist_window);

	//Update the info window as we are updated as well
	gui_update_info();
	gui_update();
}

void gui_activate_filelist(void) {
	window_activate(filelist_window);
}

void gui_deactivate_filelist(void) {
	window_deactivate(filelist_window);
}

void gui_input_filelist_return(int alt) {


	update_panels();
	doupdate();
}

void gui_input_filelist_insert(void) {

}

void gui_input_filelist(int c) {
	int alt = 0;
	int n;
	songdata_song *ftmp;
    songdata_song *selected = mp3list->selected;

	if (c == 27) {
		alt = 1;
		c = getch();
	}

	switch (c) {
		case KEY_ENTER:
		case '\n':
		case '\r':
			typed_letters[0] = '\0';
			if ((mp3list->selected->flags & F_DIR)) {
				if (!alt) {
					// change to another directory
					if (!strcmp("../", mp3list->selected->filename)) {
						char * filename = strdup(dirstack_filename());
						char * fullpath = strdup(dirstack_fullpath());
						dirstack_pop();
						songdata_read_mp3_list(mp3list, fullpath, L_NEW);
						while (strcmp(mp3list->selected->filename, filename))
							window_input(filelist_window, KEY_DOWN);
						free(filename);
						free(fullpath);
					} else {
						if (!(mp3list->flags & F_VIRTUAL))
							dirstack_push(mp3list->from, mp3list->selected->filename);
						songdata_read_mp3_list(mp3list, mp3list->selected->fullpath, L_NEW);
					}
					gui_update_filelist();
				} else {
					// add songs from directory
					if ((!(mp3list->flags & F_VIRTUAL)) & (strcmp("../", mp3list->selected->fullpath))) {
						add_to_playlist_recursive(playlist, playlist->tail, mp3list->selected);
						gui_update_playlist();
					}
				}
			} else if (mp3list->selected->flags & F_PLAYLIST) {
				if (!alt) {
					dirstack_push(mp3list->from, mp3list->selected->filename);
					songdata_read_mp3_list(mp3list, mp3list->selected->fullpath, L_NEW);
					gui_update_filelist();
				} else {
					add_to_playlist_recursive(playlist, playlist->tail, mp3list->selected);
					gui_update_playlist();
				}
			} else { // normal mp3
				if (mp3list->selected != previous_selected) {
                    if (!alt)
                        add_to_playlist(playlist, playlist->tail, mp3list->selected);
                    else
                        add_to_playlist(playlist, playlist->selected, mp3list->selected);
                    gui_update_playlist();
                    if (conf->c_flags & C_FADVANCE) {
                        window_input(filelist_window, KEY_DOWN);
                    }
                }
            }
			break;
		case KEY_LEFT:
			// leave directory
			if (!dirstack_empty()) {
				char *filename = strdup(dirstack_filename());
				char *fullpath = strdup(dirstack_fullpath());
				dirstack_pop();
				songdata_read_mp3_list(mp3list, fullpath, L_NEW);
				while (strcmp(mp3list->selected->filename, filename)) {
					window_input_list(filelist_window, KEY_DOWN);
				}
				gui_update_filelist();
				gui_update_info();
				free(filename);
				free(fullpath);
			}
			break;

		case KEY_RIGHT:
			// enter directory
			if ((mp3list->selected->flags & (F_DIR | F_PLAYLIST))
					&& (strncmp(mp3list->selected->filename, "../", 3))) {
				if (!(mp3list->flags & F_VIRTUAL)) {
					dirstack_push(mp3list->from, mp3list->selected->filename);
				}
				songdata_read_mp3_list(mp3list, mp3list->selected->fullpath, L_NEW);
				if (mp3list->head) {
					window_input_list(filelist_window, KEY_DOWN);
				}
			}
			gui_update_filelist();
			gui_update_info();
			break;
		case KEY_IC:
			if (!((mp3list->selected->flags & F_DIR) | (mp3list->selected->flags & F_PLAYLIST)))
				if (mp3list->selected != previous_selected) {
					if (playlist->playing)
						add_to_playlist(playlist, playlist->playing, mp3list->selected);
					else
						add_to_playlist(playlist, playlist->selected, mp3list->selected);
					if (conf->c_flags & C_FADVANCE) {
						window_input(filelist_window, KEY_DOWN);
						gui_update_filelist();
						gui_update_info();
					}
					gui_update_playlist();
				}
			break;
		case 'a'...'z':
		case 'A'...'Z':
		case '0'...'9':
		case '.':
		case ' ':
			ftmp = mp3list->head;
			n = 0;
			
			time_t time_now = time(NULL);
			if (difftime(time_now, typed_letters_last_activity) > 10) {
				typed_letters[0] = '\0';
			}
			
			if (strlen(typed_letters) < 4) {
				// add the letter to the string and reset the timeout
				strcat(typed_letters, (char *) &c);
			}

			while (strncasecmp(ftmp->filename, typed_letters, strlen(typed_letters)) != 0) {
				if (ftmp == mp3list->tail) {
					ftmp = NULL;
					break;
				}
				ftmp = ftmp->next;
				n++;
			}

			if (ftmp) {
				// match found
				mp3list->selected = ftmp;
				mp3list->where = n;
				gui_update_filelist();
			}
			
			typed_letters_last_activity = time_now;
			break;
		case KEY_BACKSPACE:
			if (strlen(typed_letters) >= 1) {
				typed_letters[strlen(typed_letters) - 1] = '\0';
				typed_letters_last_activity = time(NULL);
			}
			break;
		default:
			window_input(filelist_window, c);
			break;
	}
    
    previous_selected = selected;
}

void gui_shutdown_filelist(void) {
	window_free(filelist_window);
}
