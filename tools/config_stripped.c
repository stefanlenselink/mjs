/*
 * configuration file support by Trent Gamblin 9/5/1999 * * slightly modified 
 * to fix potential overflows, use color array * and add check for impossible 
 * background colors. Thanks Trent! * - WM 9/6/1999 * * More options added,
 * some rewrites. Do away with COLOR(x, y) macro. * More sanity checks on the 
 * colors, too. 
 */

#include "top.h"
#include "defs.h"
#include "struct.h"
#include "config.h"

#define COMMENT '#'
#define YESNO(s) (s[0] == 'y' || s[0] == 't' || s[0] == '1' || s[0] == 'Y' || s[0] == 'T')

static Config *set_option (Config *, char *, char *);
static int break_line (const char *, char *, char *, char *);

Config *
read_config (Config * conf)
{
	char line[1024], fname[256], *p;
	char keyword[256], param[256], value[256];
	struct stat sb;
	FILE *cfg;

	/*
	 * make sure this is all null before we go open some unknown file 
	 */
	memset (fname, 0, sizeof (fname));

	if ((p = getenv ("MJSRC")))
		strncpy (fname, p, sizeof (fname) - 1);
	else
		snprintf (fname, sizeof (fname) - 1, "%s/.mjsrc",
			  getenv ("HOME"));

	memset (&sb, 0, sizeof (sb));

	/*
	 * dont follow symlinks 
	 */
	if (((lstat (fname, &sb)) != 0) || S_ISLNK (sb.st_mode))
		return conf;
	if ((cfg = fopen (fname, "r")) == NULL)
		return conf;

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
	}
	if (conf->mp3path[1]=='\0')
		strcpy(conf->mp3path,"/\0");

	fclose (cfg);
	return conf;
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
		strncpy (conf->output, value, sizeof (conf->playlistpath) - 1);
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
	return conf;
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
