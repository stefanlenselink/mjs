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
	
	if (!(fmt) || !(file))
		return (const char *)line;

	/* check for existence of these, set default values if necessary */
	if (file == NULL)
		abort();
	if (file->artist)
		artist = file->artist;
	else
		artist = "Unknown";
	
	while (*fmt && (len < size)) {
		if (*fmt == '%') {
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
				case 'P':   /* the path without the conf->mp3path part*/
					if (file->path[0] == '/') {
						strncat(line, file->path+strlen(conf->mp3path), size-len);
						len += strlen(file->path)-strlen(conf->mp3path);
					} else {
						strncat(line, file->path, size-len);
						len += strlen(file->path);
					}
					if (len<1) {
						line = (u_char *)window->title_dfl;
						len= strlen(window->title_dfl);
					}
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
