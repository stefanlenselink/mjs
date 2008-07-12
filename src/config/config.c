/*
 * configuration file support by Trent Gamblin 9/5/1999 * * slightly modified 
 * to fix potential overflows, use color array * and add check for impossible 
 * background colors. Thanks Trent! * - WM 9/6/1999 * * More options added,
 * some rewrites. Do away with COLOR(x, y) macro. * More sanity checks on the 
 * colors, too. 
 */

#include "defs.h"
#include "extern.h"
#include "config.h"

#include <ncurses.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define COMMENT '#'
#define YESNO(s) (s[0] == 'y' || s[0] == 't' || s[0] == '1' || s[0] == 'Y' || s[0] == 'T')

static Config *set_option (Config *, char *, char *);
static u_int32_t merge_colors (u_int32_t, u_int32_t);
static u_int32_t str2color (char *);
static void set_color (char *, char *);
static void set_window_defaults (void);
static void set_color_defaults (void);
static void set_window (WindowConfig *, char *, char *);
static int break_line (const char *, char *, char *, char *);
static void parseConfig(Config * conf, char * fname);

u_int32_t * colors;

Config * conf;
void config_shutdonw(Config * conf)
{
  free(conf);
}
Config *
config_init (void)
{
	char fname[256], *p;
	/*
	 * make sure this is all null before we go open some unknown file 
	 */
    conf = malloc(sizeof (Config));
    
    strncpy (conf->mpgpath, MPGPATH, 255);
    colors = conf->colors;
	memset (colors, 0, sizeof (colors));
	set_window_defaults ();
	set_color_defaults ();
	memset (fname, 0, sizeof (fname));
    
	if ((p = getenv ("MJSRC"))){
		strncpy (fname, p, sizeof (fname) - 1);
		parseConfig(conf, fname);
	}
	memset (fname, 0, sizeof (fname));
	snprintf (fname, sizeof (fname) - 1, "%s/.mjsrc", getenv ("HOME"));
	parseConfig(conf, fname);
	memset (fname, 0, sizeof (fname));
	snprintf (fname, sizeof (fname) - 1, "/etc/mjsrc");
	parseConfig(conf, fname);
//defaults :
	if (conf->output[1]=='\0' && conf->snd_system[1]=='\0')
		strcpy(conf->output,"/dev/dsp\0");
	if (conf->mpgpath[1]=='\0')
		strcpy(conf->mpgpath,"mpg123\0");
	if (conf->mp3path[1]=='\0')
		strcpy(conf->mp3path,"/\0");

	return conf;
}

static void parseConfig(Config * conf, char * fname)
{
	struct stat sb;
	char keyword[256], param[256], value[256], line[1024];
	FILE *cfg;
	memset (&sb, 0, sizeof (sb));

	/*
	 * dont follow symlinks 
	 */
	if (((lstat (fname, &sb)) != 0) || S_ISLNK (sb.st_mode))
		return;
	if ((cfg = fopen (fname, "r")) == NULL)
		return;

	while (fgets (line, sizeof (line), cfg))
	{
		if (*line == COMMENT || *line == '\n')
			continue;
		memset (keyword, 0, sizeof (keyword));
		memset (param, 0, sizeof (param));
		memset (value, 0, sizeof (value));
		if (break_line (line, keyword, param, value) < 3)
			continue;
		if (!strcasecmp (keyword, "set"))
			set_option (conf, param, value);
		else if (!strcasecmp (keyword, "color"))
			set_color (param, value);
		else if (!strcasecmp (keyword, "info"))
			set_window (&conf->info_window, param, value);
		else if (!strcasecmp (keyword, "files"))
			set_window (&conf->files_window, param, value);
		else if (!strcasecmp (keyword, "play"))
			set_window (&conf->play_window, param, value);
		else if (!strcasecmp (keyword, "menubar"))
			set_window (&conf->menubar_window, param, value);
		else if (!strcasecmp (keyword, "playback"))
			set_window (&conf->playback_window, param, value);
	}
	fclose (cfg);
}

/*
 * All configurable parameters go here 
 */

static Config *
set_option (Config * conf, char *option, char *value)
{
	char *p;
	if (!strcasecmp (option, "mpgpath"))
		strncpy (conf->mpgpath, value, sizeof (conf->mpgpath) - 1);
	else if (!strcasecmp (option, "mp3dir")) {
		strncpy (conf->mp3path, value, sizeof (conf->mp3path) - 1);
		if (conf->mp3path[strlen(conf->mp3path)-1] != '/')
			strcat(conf->mp3path,"/\0");
	} else if (!strcasecmp (option, "statefile"))
		strncpy (conf->statefile, value, sizeof (conf->statefile) - 1);
	else if (!strcasecmp (option, "logfile"))
		strncpy (conf->logfile, value, sizeof (conf->logfile) - 1);
	else if (!strcasecmp (option, "resultsfile"))
		strncpy (conf->resultsfile, value, sizeof (conf->resultsfile) - 1);
	else if (!strcasecmp (option, "playlistpath"))
		strncpy (conf->playlistpath, value, sizeof (conf->playlistpath) - 1);
	else if (!strcasecmp (option, "output_device"))
		strncpy (conf->output, value, sizeof (conf->output) - 1);
	else if (!strcasecmp (option, "sound_system"))
		strncpy (conf->snd_system, value, sizeof (conf->snd_system) - 1);
	else if (!strcasecmp (option, "file_advance"))
		conf->c_flags |= YESNO (value) * C_FADVANCE;
	else if (!strcasecmp (option, "playlist_advance"))
		conf->c_flags |= YESNO (value) * C_PADVANCE;
	else if (!strcasecmp (option, "loop"))
		conf->c_flags |= YESNO (value) * C_LOOP;
	else if (!strcasecmp (option, "playlists_to_files"))
		conf->c_flags |= YESNO (value) * C_P_TO_F;
	else if (!strcasecmp (option, "mono_output"))
		conf->c_flags |= YESNO (value) * C_MONO;
	else if (!strcasecmp (option, "allow_playlist_saving"))
		conf->c_flags |= YESNO (value) * C_ALLOW_P_SAVE;
	else if (!strcasecmp (option, "keep_playlist_at_exit"))
		conf->c_flags |= YESNO (value) * C_P_SAVE_EXIT;
	else if (!strcasecmp (option, "show_track_numbers"))
		conf->c_flags |= YESNO (value) * C_TRACK_NUMBERS;
	else if (!strcasecmp (option, "use_genre"))
		conf->c_flags |= YESNO (value) * C_USE_GENRE;
	else if (!strcasecmp (option, "fix_window_borders"))
	{
		switch (value[0]) {
			case 'A' :
			case 'a' :
				if ((p = getenv ("TERM")))
					if (!strcasecmp(p,"xterm"))
						conf->c_flags |= C_FIX_BORDERS;
				break;
			case 'Y' :
			case 'y' :
			case 'T' :
			case 't' :
				conf->c_flags |= C_FIX_BORDERS;
				break;
		}
	}
	else if (!strcasecmp (option, "buffer"))
	{
		errno = 0;
		conf->buffer = strtoul (value, NULL, 10);
		if (errno)
			conf->buffer = 0;
	}
	else if (!strcasecmp (option, "jump"))
	{
		errno = 0;
		conf->jump = strtoul (value, NULL, 10);
		if (errno)
			conf->jump = 1000;
	}
        else if (!strcasecmp (option, "refresh_interval"))
	{
		errno = 0;
		conf->refresh_interval = strtoul (value, NULL, 10);
		if (errno)
			conf->refresh_interval = 60;
	}
	return conf;
}


static u_int32_t
str2color (char *color)
{
	while (color[0] == ' ')	// remove whitespace at start
		color++;
	if (!strncasecmp (color, "black",5))
		return BLACK;
	else if (!strncasecmp (color, "red",3))
		return RED;
	else if (!strncasecmp (color, "green",5))
		return GREEN;
	else if (!strncasecmp (color, "brown",5))
		return BROWN;
	else if (!strncasecmp (color, "blue",4))
		return BLUE;
	else if (!strncasecmp (color, "magenta",7))
		return MAGENTA;
	else if (!strncasecmp (color, "cyan",4))
		return CYAN;
	else if (!strncasecmp (color, "grey",4))
		return GREY;
	else if (!strncasecmp (color, "b_black",7))
		return B_BLACK;
	else if (!strncasecmp (color, "b_red",5))
		return B_RED;
	else if (!strncasecmp (color, "b_green",7))
		return B_GREEN;
	else if (!strncasecmp (color, "yellow",6))
		return YELLOW;
	else if (!strncasecmp (color, "b_blue",6))
		return B_BLUE;
	else if (!strncasecmp (color, "b_magenta",9))
		return B_MAGENTA;
	else if (!strncasecmp (color, "b_cyan",6))
		return B_CYAN;
	else if (!strncasecmp (color, "white",5))
		return WHITE;
	return GREY;
}

static u_int32_t
merge_colors (u_int32_t fore, u_int32_t back)
{
	/*
	 * make sure they aren't dumb enough to use a bold background, and
	 * attempt to correct for it.
	 */
	if (back > GREY)
		back &= ~A_BOLD;
	if (fore == GREY && back == BLACK)
		return COLOR_PAIR (0);
	else if (fore == BLACK && back == BLACK)
		return COLOR_PAIR (56);
	else
		return (fore << 11 | back << 8);
}

static void
set_color (char *color, char *value)
{
	char *fore = value;
	char *back = strchr (value, ':');

	if (back == NULL)
		back = "black\0";
	else 
		back++;

	/* IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT 
	* check
	* u_int32_t colors[24];
	* around line 24 !!!!!
	* resize this array when adding more color entries !!!!!
	*/



	if (!strcasecmp (color, "window_active"))
		colors[WIN_ACTIVE] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "window_active_title"))
		colors[WIN_ACTIVE_TITLE] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "window_active_scroll"))
		colors[WIN_ACTIVE_SCROLL] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "window_active_scroll_bar"))
		colors[WIN_ACTIVE_SCROLLBAR] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "window_inactive"))
		colors[WIN_INACTIVE] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "window_inactive_title"))
		colors[WIN_INACTIVE_TITLE] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "window_inactive_scroll"))
		colors[WIN_INACTIVE_SCROLL] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "window_inactive_scroll_bar"))
		colors[WIN_INACTIVE_SCROLLBAR] =
			merge_colors (str2color (fore), str2color (back));

	else if (!strcasecmp (color, "files_selected_file"))
		colors[FILE_SELECTED] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "files_unselected_file"))
		colors[FILE_UNSELECTED] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "files_unselected_directory"))
		colors[FILE_UNSELECTED_DIRECTORY] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "files_selected_directory"))
		colors[FILE_SELECTED_DIRECTORY] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "files_background"))
		colors[FILE_WINDOW] =
			merge_colors (BLACK, str2color (fore));

	else if (!strcasecmp (color, "playlist_unselected"))
		colors[PLAY_UNSELECTED] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "playlist_unselected_playing"))
		colors[PLAY_UNSELECTED_PLAYING] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "playlist_selected_playing"))
		colors[PLAY_SELECTED_PLAYING] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "playlist_selected"))
		colors[PLAY_SELECTED] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "playlist_background"))
		colors[PLAY_WINDOW] =
			merge_colors (BLACK, str2color (fore));

	else if (!strcasecmp (color, "info_text"))
		colors[INFO_TEXT] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "info_background"))
		colors[INFO_WINDOW] =
			merge_colors (BLACK, str2color (fore));

	else if (!strcasecmp (color, "menu_background"))
		colors[MENU_WINDOW] =
			merge_colors (BLACK, str2color (fore));
	else if (!strcasecmp (color, "menu_text"))
		colors[MENU_TEXT] =
			merge_colors (str2color (fore), str2color (back));

	else if (!strcasecmp (color, "playback_text"))
		colors[PLAYBACK_TEXT] =
			merge_colors (str2color (fore), str2color (back));
	else if (!strcasecmp (color, "playback_background"))
		colors[PLAYBACK_WINDOW] =
			merge_colors (BLACK, str2color (fore));
}

/*
 * Return the value of expression e. * Evaluation is left to right and
 * opererators + - / * are supported. * 'w' and 'h' are replaced by the
 * screen width and height. * -1 is returned on error. 
 */
static int
window_calc (char *e)
{
	int result = 0, tmp = 0, op = 0;
	char *s = NULL;

	while (*e != '\0')
	{
		tmp = 0;
		s = NULL;
		if (isspace (*e))
			e++;
		else if (*e == 'h')
		{
			e++;
			tmp = LINES;
		}
		else if (*e == 'w')
		{
			e++;
			tmp = COLS;
		}
		else if (isdigit (*e))
		{
			tmp = strtoul (e, &s, 10);
			e = s;
		}
		else
		{
			op = *e++;
			continue;
		}
		if (op)
		{
			switch (op)
			{
			case '+':
				result += tmp;
				op = 0;
				break;
			case '-':
				result -= tmp;
				op = 0;
				break;
			case '*':
				result *= tmp;
				op = 0;
				break;
			case '/':
				result /= tmp;
				op = 0;
				break;
			default:
				return -1;
			}
		}
		else if (tmp)
			result = tmp;
	}
	return result;
}

static void
set_window (WindowConfig * win, char *param, char *value)
{
	int *p = NULL, result;

	switch (*param)
	{
	case 'h':
		p = &win->height;
		break;
	case 'w':
		p = &win->width;
		break;
	case 'y':
		p = &win->y;
		break;
	case 'x':
		p = &win->x;
		break;
	case 't':
		if (!strcasecmp (param, "title.default"))
			win->title_dfl = strdup (value);
		else if (!strcasecmp (param, "title.format"))
			win->title_fmt = strdup (value);
		return;
	case 'f':
		win->format = strdup (value);
		return;
	default:
		return;
	}
	/*
	 * -- not needed until some options start with the same letter :) if
	 * (!strcasecmp(param, "height")) p = &win->height; else if
	 * (!strcasecmp(param, "width")) p = &win->width; else if
	 * (!strcasecmp(param, "y")) p = &win->y; else if (!strcasecmp(param,
	 * "x")) p = &win->x; else if (!strcasecmp(param, "title")) win->title =
	 * strdup(value); else if (win->flags & W_LIST && !strcasecmp(param,
	 * "format")) win->format = strdup(value); else return; 
	 */
	if (p && ((result = window_calc (value)) >= 0))
		*p = result;
}


static void
set_window_defaults (void)
{
	conf->files_window.height = LINES - 1, conf->files_window.width = COLS / 4, conf->files_window.y =
		conf->files_window.x = 0;
	conf->files_window.title_dfl = "MP3  Files", conf->files_window.format = "%f";
    conf->info_window.height = 8, conf->info_window.width = 0, conf->info_window.y = 0, conf->info_window.x = COLS / 4;
    conf->info_window.title_dfl = "MP3 Info";
    conf->play_window.height = LINES - 9, conf->play_window.width = 0, conf->play_window.y = 8, conf->play_window.x =
		COLS / 4;
	conf->play_window.title_dfl = "Playlist", conf->play_window.format = "%f";
    conf->menubar_window.height = 1, conf->menubar_window.width = 0, conf->menubar_window.y =
		LINES - 1, conf->menubar_window.x = 0;
	conf->playback_window.height = 3, conf->playback_window.width = 0, conf->playback_window.y =
        6, conf->playback_window.x = COLS / 4;
	conf->playback_window.title_dfl = "Playback Info";
    conf->playback_window.title_fmt = "%t";
    conf->menubar_window.title_dfl = "You don't have .mjsrc in your home dir - MP3 Jukebox System";

}

static void
set_color_defaults (void)
{
	colors[WIN_ACTIVE] = merge_colors (B_RED, BLACK);
	colors[WIN_INACTIVE] = merge_colors (B_BLUE, BLACK);
	colors[WIN_INACTIVE_TITLE] = merge_colors (B_BLUE, BLACK);
	colors[WIN_INACTIVE_SCROLL] = merge_colors (BLUE, BLACK);
	colors[WIN_INACTIVE_SCROLLBAR] = merge_colors (B_BLUE, BLACK);
	colors[WIN_ACTIVE_TITLE] = merge_colors (B_RED, BLACK);
	colors[WIN_ACTIVE_SCROLL] = merge_colors (RED, BLACK);
	colors[WIN_ACTIVE_SCROLLBAR] = merge_colors (B_RED, BLACK);

	colors[FILE_UNSELECTED] = merge_colors (B_BLUE, BLACK);
	colors[FILE_SELECTED] = merge_colors (B_BLUE, RED);
	colors[FILE_UNSELECTED_DIRECTORY] = merge_colors (B_BLUE, BLACK);
	colors[FILE_SELECTED_DIRECTORY] = merge_colors (B_BLUE, RED);
	colors[FILE_WINDOW] = merge_colors (BLACK, BLACK);

	colors[PLAY_UNSELECTED] = merge_colors (B_BLUE, BLACK);
	colors[PLAY_SELECTED] = merge_colors (B_BLUE, RED);
	colors[PLAY_UNSELECTED_PLAYING] = merge_colors (B_RED, BLUE);
	colors[PLAY_SELECTED_PLAYING] = merge_colors (B_RED, RED);
	colors[PLAY_WINDOW] = merge_colors (BLACK, BLACK);

	colors[INFO_TEXT] = merge_colors (B_BLUE, BLACK);
	colors[INFO_WINDOW] = merge_colors (BLACK, BLACK);

	colors[MENU_TEXT] = merge_colors (B_BLUE, BLACK);
	colors[MENU_WINDOW] = merge_colors (BLACK, BLACK);

	colors[PLAYBACK_TEXT] = merge_colors (B_BLUE, BLACK);
	colors[PLAYBACK_WINDOW] = merge_colors (BLACK, BLACK);
}

static int
break_line (const char *line, char *keyword, char *param, char *value)
{
	char *p = (char *) line, *s = keyword;
	int i = 0;

	while (isspace (*p))
		p++;
	if (!*p)
		return 0;
	while (i++ < 256 && !isspace (*p))
		*s++ = *p++;
	while (isspace (*p))
		p++;
	if (!*p)
		return 1;
	i = 0;
	s = param;
	while (i++ < 256 && !isspace (*p))
		*s++ = *p++;
	while (isspace (*p))
		p++;
	if (!*p)
		return 2;
	i = 0;
	s = value;
	while (i++ < 256 && isprint (*p))
		*s++ = *p++;
	return 3;
}
