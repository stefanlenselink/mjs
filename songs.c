#include "defs.h"
#include "mms.h"
#include "struct.h"
#include "proto.h"
#include "extern.h"
#include "songs.h"

flist *
read_songs(flist *songs)
{
	char buf[MAXLEN+1], *p, *s;
	flist *ftmp;
	FILE *sf;
	sf = fopen(SONGFILE, "r");
	if (!sf)
		return songs;
	while (!feof(sf)) {
		memset(buf, 0, MAXLEN+1);
		if (!(fgets(buf, MAXLEN+1, sf)))
			continue;
		if (!*buf)
			continue;
		else if (*buf == '#')
			continue;
		else if (buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';
		if (!(p = strchr(buf, ':')))
			continue;
		*p++ = '\0';
		for (ftmp = songs; ftmp; ftmp = ftmp->next) {
			if (!strcasecmp(buf, ftmp->filename))
				break;
		}
		if (!ftmp)
			continue;
		if ((s = strchr(p, ':')) && (s != p)) {
			*s++ = '\0';
			ftmp->title = strdup(p);
		}
		if (s && *s && *s != ':')
			ftmp->artist = strdup(s);
	}
	fclose(sf);
	return songs;
}

wlist *
read_playlist (wlist *plist, const char *file)
{
	FILE *fp;
	flist *ftmp = NULL, *last = NULL;
	wlist *playlist = plist;
	char buf[1025];

	if (!plist)
		return plist;

	if (!(fp = fopen(file, "r")))
		return plist;
	plist = (wlist *)calloc(1, sizeof(wlist));

	if (playlist->head && pid)
		stop_player(playlist);
	memset(buf, 0, sizeof(buf));
	if (!fgets(buf, 1025, fp)) {
		fclose(fp);
		free(plist);
		return playlist;
	}
	last = (flist *)calloc(1, sizeof(flist));
	if (!(plist->head = plist->top = plist->selected = parse_playlist_line(last, buf)))
		free_flist(last);
	else {
		plist->length++;
		last->flags |= F_SELECTED;
	}
	while (!feof(fp)) {
		memset(buf, 0, sizeof(buf));
		if (!fgets(buf, 1025, fp))
			break;
		ftmp = (flist *)calloc(1, sizeof(flist));
		if (!(parse_playlist_line(ftmp, buf))) {
			free_flist(ftmp);
			continue;
		}
		ftmp->prev = last;
		last->next = ftmp;
		last = ftmp;
		plist->length++;
	}
	plist->tail = ftmp;
	fclose(fp);
	if (plist->length > 0) {
		free_playlist(playlist);
		playlist = plist;
	} else
		free(plist);
	playlist->where = 1;
	return playlist;
}

/* 
 * parse the line. If we encounter an error, give up and tell the caller its
 * their problem.
 */

static flist *
parse_playlist_line (flist *file, char *line)
{
	char *s, *p = line;

	if (!(s = strchr(line, ':')))
		return NULL;
	*s++ = '\0';
	if (!(p = strrchr(p, '/')))
		return NULL;
	*p++ = '\0';
	file->path = strdup(line);
	file->filename = strdup(p);
	if (!(p = strchr(s, ':')))
		return NULL;
	*p++ = '\0';
	if (s && *s)
		file->artist = strdup(s);
	if (!(s = strchr(p, ':')))
		return NULL;
	*s++ = '\0';
	if (p && *p)
		file->title = strdup(p);
	if (!(p = strchr(s, ':')))
		return NULL;
	file->length = (time_t) strtoul(s, &p, 10);
	if (*p != ':')
		return NULL;
	file->bitrate = (short) strtoul(++p, &s, 10);
	if (*s != ':')
		return NULL;
	file->frequency = (int) strtoul(++s, &p, 10);
	return file;
}

static void
free_playlist (wlist *playlist)
{
	flist *ftmp;
	
	if (!playlist)
		return;
	for (ftmp = playlist->head; ftmp; ftmp = ftmp->next) {
		free(ftmp->filename);
		free(ftmp->path);
		free(ftmp->title);
		free(ftmp->artist);
		free(ftmp->prev);
	}
	free(ftmp);
	memset(playlist, 0, sizeof(playlist));
}

static void
free_flist (flist *file)
{
	if (!file)
		return;
	free(file->filename);
	free(file->path);
	free(file->title);
	free(file->artist);
	free(file);
}

int
write_playlist (wlist *playlist, const char *file)
{
	FILE *fp;
	flist *ftmp;
	
	if (!playlist || !playlist->head)
		return 0;
	if (!(fp = fopen(file, "w")))
		return 0;
	
	for (ftmp = playlist->head; ftmp; ftmp = ftmp->next)
		fprintf(fp, "%s/%s:%s:%s:%ld:%d:%d\n", ftmp->path, ftmp->filename,
			ftmp->artist?:"", ftmp->title?:"", (long int)ftmp->length, ftmp->bitrate, ftmp->frequency);
	fclose(fp);
	return 1;
}
