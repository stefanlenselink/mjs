/* configuration file support by Trent Gamblin 9/5/1999
 *
 * slightly modified to fix potential overflows, use color array
 * and add check for impossible background colors. Thanks Trent!
 * - WM 9/6/1999
 * 
 * More options added, some rewrites. Do away with COLOR(x, y) macro.
 * More sanity checks on the colors, too.
 */

#include "mms.h"
#include "defs.h"
#include "colors.h"
#include "struct.h"
#include "extern.h"

#define COMMENT '#'
#define YESNO(s) (s[0] == 'y' || s[0] == 't' || s[0] == '1')

static Config *set_option (Config *, char *, char *);
static u_int32_t merge_colors(u_int32_t, u_int32_t);
static u_int32_t str2color(char *);
static void set_color(char *, char *);
static void set_window_defaults(void);
static void set_color_defaults(void);
static void set_window(char *, char *);

u_int32_t colors[20];

Config *
read_config(Config *conf)
{
	char line[1024], fname[256], *p;
	char keyword[32], param[32], value[256];
	struct stat sb;
	FILE *cfg;

	/* make sure this is all null before we go open some unknown file */
	memset(fname, 0, sizeof(fname));
	memset(colors, 0, sizeof(colors));

	set_window_defaults();
	set_color_defaults();

	if ((p = getenv("MMSRC")))
		strncpy(fname, p, sizeof(fname)-1);
	else
		snprintf(fname, sizeof(fname)-1, "%s/.mmsrc", getenv("HOME"));
	
	memset(&sb, 0, sizeof(sb));

	/* dont follow symlinks */
	if (((lstat(fname, &sb)) != 0) || S_ISLNK(sb.st_mode))
		return conf;		
	if ((cfg = fopen(fname, "r")) == NULL)
		return conf;

	while (fgets(line, sizeof(line), cfg)) {
		if (*line == COMMENT)
			continue;
		if (sscanf(line, "%s %s %s", keyword, param, value) != 3)
			continue;
		if (!strcasecmp(keyword, "set"))
			set_option(conf, param, value);
		else if (!strcasecmp(keyword, "color"))
			set_color(param, value);
		else if (!strcasecmp(keyword, "window"))
			set_window(param, value);
	}
	fclose(cfg);
	return conf;
}

/* All configurable parameters go here */

static Config *
set_option(Config *conf, char *option, char *value)
{
	if (!strcasecmp(option, "mpgpath"))
		strncpy(conf->mpgpath, value, sizeof(conf->mpgpath)-1);
	if (!strcasecmp(option, "playlist"))
		strncpy(conf->dfl_plist, value, sizeof(conf->mpgpath)-1);
	else if (!strcasecmp(option, "file_advance"))
		conf->f_advance = YESNO(value);
	else if (!strcasecmp(option, "playlist_advance"))
		conf->p_advance = YESNO(value);
	else if (!strcasecmp(option, "skip_info_box"))
		conf->skip_info = YESNO(value);
	else if (!strcasecmp(option, "loop"))
		conf->loop = YESNO(value);
	else if (!strcasecmp(option, "buffer")) {
		errno = 0;
		conf->buffer = strtoul(value, NULL, 10);
		if (errno)
			conf->buffer = 0;
	}
	else if (!strcasecmp(option, "jump")) {
		errno = 0;
		conf->jump = strtoul(value, NULL, 10);
		if (errno)
			conf->jump = 1000;
	}
	return conf;
}


static u_int32_t
str2color(char *color)
{
	if (!strcasecmp(color, "black")) return BLACK;
	else if (!strcasecmp(color, "red")) return RED;
	else if (!strcasecmp(color, "green")) return GREEN;
	else if (!strcasecmp(color, "brown")) return BROWN;
	else if (!strcasecmp(color, "blue")) return BLUE;
	else if (!strcasecmp(color, "magenta")) return MAGENTA;
	else if (!strcasecmp(color, "cyan")) return CYAN;
	else if (!strcasecmp(color, "grey")) return GREY;
	else if (!strcasecmp(color, "b_black")) return B_BLACK;
	else if (!strcasecmp(color, "b_red")) return B_RED;
	else if (!strcasecmp(color, "b_green")) return B_GREEN;
	else if (!strcasecmp(color, "yellow")) return YELLOW;
	else if (!strcasecmp(color, "b_blue")) return B_BLUE;
	else if (!strcasecmp(color, "b_magenta")) return B_MAGENTA;
	else if (!strcasecmp(color, "b_cyan")) return B_CYAN;
	else if (!strcasecmp(color, "white")) return WHITE;
	return GREY;
}

static u_int32_t
merge_colors(u_int32_t fore, u_int32_t back)
{
	/*
	 * make sure they aren't dumb enough to use a bold background, and
	 * attempt to correct for it.
	 */
	if (back > GREY)
		back &= ~A_BOLD;
	if (fore == GREY && back == BLACK)
		return COLOR_PAIR(0);
	else if (fore == BLACK && back == BLACK)
		return COLOR_PAIR(56);
	else
		return (fore<<11 | back<<8);
}

static void
set_color(char *color, char *value)
{
	char *fore = value;
	char *back = strchr(value, ':');

	if (back == NULL)
		return;
	*back++ = '\0';

	if (!strcasecmp(color, "active"))
		colors[ACTIVE] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "inactive"))
		colors[INACTIVE] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "selected"))
		colors[SELECTED] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "unselected"))
		colors[UNSELECTED] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "title"))
		colors[TITLE] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "scroll"))
		colors[SCROLL] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "scroll_bar"))
		colors[SCROLLBAR] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "playing"))
		colors[PLAYING] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "sel_playing"))
		colors[SEL_PLAYING] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "file_back"))
		colors[FILE_BACK] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "info_back"))
		colors[INFO_BACK] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "play_back"))
		colors[PLAY_BACK] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "menu_back"))
		colors[MENU_BACK] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "menu_text"))
		colors[MENU_TEXT] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "arrows"))
		colors[ARROWS] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "edit_back"))
		colors[EDIT_BACK] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "edit_active"))
		colors[EDIT_ACTIVE] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "edit_inactive"))
		colors[EDIT_INACTIVE] = merge_colors(str2color(fore), str2color(back));
	else if (!strcasecmp(color, "edit_prompt"))
		colors[EDIT_PROMPT] = merge_colors(str2color(fore), str2color(back));
}

/* Return the value of expression e.
 * Evaluation is left to right and opererators + - / * are supported.
 * 'w' and 'h' are replaced by the screen width and height.
 * -1 is returned on error.
 */
static int window_calc(char *e)
{
	int result = 0, tmp = 0, op = 0;
	char *s = NULL;

	while (*e != '\0') {
		tmp = 0;
		s = NULL;
		if (isspace(*e))
			e++;
		else if (*e == 'h') {
			e++;
			tmp = LINES;
		} else if (*e == 'w') {
			e++;
			tmp = COLS;
		}	else if (isdigit(*e)) {
			tmp = strtoul(e, &s, 10);
			e = s;
		} else {
			op = *e++;
			continue;
		}
		if (op) {
			switch (op) {
				case '+': result += tmp; op = 0; break;
				case '-': result -= tmp; op = 0; break;
				case '*': result *= tmp; op = 0; break;
				case '/': result /= tmp; op = 0; break;
				default: return -1;
			}
		} else if (tmp)
			result = tmp;
	}
	return result;
}

static void set_window(char *param, char *value)
{
	int *p = NULL, result;

	if (!strcasecmp(param, "files_height")) p = &files->height;
	else if (!strcasecmp(param, "files_width")) p = &files->width;
	else if (!strcasecmp(param, "files_y")) p = &files->y;
	else if (!strcasecmp(param, "files_x")) p = &files->x;
	else if (!strcasecmp(param, "info_height")) p = &info->height;
	else if (!strcasecmp(param, "info_width")) p = &info->width;
	else if (!strcasecmp(param, "info_y")) p = &info->y;
	else if (!strcasecmp(param, "info_x")) p = &info->x;
	else if (!strcasecmp(param, "play_height")) p = &play->height;
	else if (!strcasecmp(param, "play_width")) p = &play->width;
	else if (!strcasecmp(param, "play_y")) p = &play->y;
	else if (!strcasecmp(param, "play_x")) p = &play->x;
	else if (!strcasecmp(param, "menubar_height")) p = &menubar->height;
	else if (!strcasecmp(param, "menubar_width")) p = &menubar->width;
	else if (!strcasecmp(param, "menubar_y")) p = &menubar->y;
	else if (!strcasecmp(param, "menubar_x")) p = &menubar->x;
	else if (!strcasecmp(param, "id3box_height")) p = &id3box->height;
	else if (!strcasecmp(param, "id3box_width")) p = &id3box->width;
	else if (!strcasecmp(param, "id3box_y")) p = &id3box->y;
	else if (!strcasecmp(param, "id3box_x")) p = &id3box->x;
	else return;

	if ((result = window_calc(value)) >= 0)
		*p = result;
}


static void set_window_defaults(void)
{
	files->height = LINES-1, files->width = COLS/4, files->y = files->x = 0;
	info->height = 8, info->width = 0, info->y = 0, info->x = COLS/4;
	play->height = LINES-9, play->width = 0, play->y = 8, play->x = COLS/4;
	menubar->height = 1, menubar->width = 0, menubar->y = LINES-1, menubar->x = 0;
	id3box->height = 8, id3box->width = 60, id3box->y = 9, id3box->x = 15;
}

static void set_color_defaults(void)
{
	colors[ACTIVE] = merge_colors(B_GREEN, BLUE);
	colors[INACTIVE] = merge_colors(YELLOW, BLUE);
	colors[SELECTED] = merge_colors(YELLOW, RED);
	colors[UNSELECTED] = merge_colors(WHITE, BLUE); 
	colors[TITLE] = merge_colors(WHITE, GREEN);
	colors[SCROLL] = merge_colors(YELLOW, BLUE);
	colors[SCROLLBAR] = merge_colors(YELLOW, BLUE);
	colors[PLAYING] = merge_colors(B_CYAN, BLUE);
	colors[SEL_PLAYING] = merge_colors(B_CYAN, RED);
	colors[FILE_BACK] = merge_colors(BLACK, BLUE);
	colors[INFO_BACK] = merge_colors(BLACK, BLUE);
	colors[PLAY_BACK] = merge_colors(BLACK, BLUE);
	colors[MENU_BACK] = merge_colors(BLACK, BLUE);
	colors[MENU_TEXT] = merge_colors(B_RED, BLUE);
	colors[ARROWS] = merge_colors(YELLOW, BLUE);
	colors[EDIT_BACK] = merge_colors(B_GREEN, BLUE);
	colors[EDIT_ACTIVE] = merge_colors(WHITE, BLACK);
	colors[EDIT_INACTIVE] = merge_colors(YELLOW, BLUE);
	colors[EDIT_PROMPT] = merge_colors(WHITE, BLUE);
}
