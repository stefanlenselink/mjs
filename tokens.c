#include "top.h"
#include "defs.h"
#include "struct.h"
#include "tokens.h"
#include "extern.h"

const char *
parse_tokens(Window *window, flist *file, char *line, int size, const char *fmt)
{
	int len = 0;
	char *artist;
	
	if (!fmt || !file)
		return (const char *)line;

	/* check for existence of these, set default values if necessary */
	
	if (file->artist)
		artist = file->artist;
	else
		artist = "Unknown";
	
	while (*fmt && (len < size)) {
		if (*fmt == '%') {
			int min;
			double sec;
			switch(*++fmt) {
				case 't':   /* the song title */
					strncat(line, file->filename, size-len);
					len += strlen(file->filename);
					break;
				case 'a':   /* the artist */
					strncat(line, artist, size-len);
					len += strlen(artist);
					break;
				case 'f':   /* filename */
					strncat(line, file->filename, size-len);
					len += strlen(file->filename);
					break;
				case 'F':   /* complete filename, incl path */
					strncat(line, file->fullpath, size-len);
					len += strlen(file->fullpath);
					break;
				case 'p':   /* the path */
					strncat(line, file->path, size-len);
					len += strlen(file->path);
					break;
				case 'm':   /* how many minutes? */
					len += snprintf(line+len, size-len, "%ld", file->length / 60);				
					break;
				case 's':   /* "remainder" seconds (fraction of minutes) */
					len += snprintf(line+len, size-len, "%ld", file->length % 60);				
					break;
				case 'S':   /* total length in seconds */
					len += snprintf(line+len, size-len, "%ld", file->length);				
					break;
				case 'T':   /* time in format [%m:%s] */
					min = file->length / 60;
					sec = (double)file->length - min * 60;
					len += snprintf(line+len, size-len, "[%02d:%02.0f]", min, sec);
					break;
				case '%':
					line[len++] = '%';
					break;
				default:
					break;
			}
			fmt++;
		} else {
			line[len++] = *fmt++;
		}
	}
	return (const char *)line;
}
