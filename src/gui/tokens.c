#include "defs.h"
#include "tokens.h"


#include <string.h>

const char *
parse_tokens ( Window *window, songdata_song *file, char *line, int size, const char *fmt )
{
	int len = 0;
	char *artist;

	if ( ! ( fmt ) || ! ( file ) )
		return ( const char * ) line;

	/* check for existence of these, set default values if necessary */
	if ( file->artist )
		artist = file->artist;
	else
		artist = "Unknown";

	while ( *fmt && ( len < size ) )
	{
		if ( *fmt == '%' )
		{
			switch ( *++fmt )
			{
				case 't':   /* the song title */
					strncat ( line, file->filename, size-len );
					len += strlen ( file->filename );
					break;
				case 'a':   /* the artist */
					strncat ( line, artist, size-len );
					len += strlen ( artist );
					break;
				case 'f':   /* filename */
					strncat ( line, file->filename, size-len );
					len += strlen ( file->filename );
					break;
				case 'F':   /* complete filename, incl path */
					strncat ( line, file->fullpath, size-len );
					len += strlen ( file->fullpath );
					break;
				case 'p':   /* the path */
					strncat ( line, file->fullpath, size-len );
					len += strlen ( file->fullpath );
					break;
				case 'l':   /* the actual place and length of the platlist. */
					len += snprintf ( line+len, size-len, "(%d/", window->contents.list->where );
					len += snprintf ( line+len, size-len, "%d)", window->contents.list->length );
					break;
				case 'P':   /* the path without the conf->mp3path part */
					if ( file->flags & F_DIR )
					{
						strncat ( line, file->relpath, size-len );
						len += strlen ( file->relpath );
					}
					else
					{
						strncat ( line, file->path, size-len );
						len += strlen ( file->path );
					}
					break;
				case '%':
					line[len++] = '%';
					break;
				default:
					break;
			}
			fmt++;
		}
		else
		{
			line[len++] = *fmt++;
		}
	}
	return ( const char * ) line;
}
