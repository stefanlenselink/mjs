#include "mms.h"
#include "defs.h"
#include "struct.h"
#include "extern.h"
#include "proto.h"

static ID3tag id3;
static my_tag new_tag;
static int count = 0;
static int had_tag = 0;
static char *filename;

char *Genres[255] = {
	"Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
	"Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
	"Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
	"Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient",
	"Trip-Hop", "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical",
	"Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise",
	"AlternRock", "Bass", "Soul", "Punk", "Space", "Meditative",
	"Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic", "Darkwave",
	"Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream",
	"Southern Rock", "Comedy", "Cult", "Gangsta", "Top 40", "Christian Rap",
	"Pop/Funk", "Jungle", "Native American", "Cabaret",	"New Wave",
	"Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",	"Tribal",
	"Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll",
	"Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing", "Fast-Fusion",
	"Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde", 
	"Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock",
	"Slow Rock", "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour",
	"Speech", "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony",
	"Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam", "Club",
	"Tango", "Samba", "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul",
	"Freestyle", "Duet", "Punk Rock", "Drum Solo", "A capella", "Euro-House",
	"Dance Hall", "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
	"Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
	"Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover",
	"Contemporary C", "Christian Rock", "Merengue", "Salsa", "Thrash Metal",
	"Anime", "JPop", "SynthPop"
};

static void edit_next(Input *);
static int edit_field_finish(Input *);
static void init_id3_box (Window *);
static int id3_input (Input *, int, int);
static int update_edit_wrapper (Input *);
static int finish_edit (void);

static int read_id3(char *filename, ID3tag *tag)
{
	
	int fd = open(filename, O_RDONLY);
	
	had_tag = 0;
	if ((fd == -1) || (lseek(fd, -128, SEEK_END) == -1))
		return had_tag;
	had_tag = -sizeof(ID3tag); /* assume we have a tag */
	memset(tag, 0, sizeof(ID3tag));
	if ((read(fd, tag, 128) == 128) && strncmp(tag->tag, "TAG", 3)) {
		memset(tag, 0, sizeof(ID3tag));
		strncpy(tag->tag, "TAG", 3);
		had_tag = 0; /* i guess we were wrong :) */
	}
	close(fd);
	return had_tag;
}

static void write_id3(char *filename, ID3tag *tag)
{
	int fd = open(filename, O_RDWR);

	if ((fd > -1) && (lseek(fd, had_tag, SEEK_END) > -1)) {
		write(fd, tag, sizeof(ID3tag));
		close(fd);
	}
}

static int genre_number(char *name)
{
	int i;

	for (i = 0; Genres[i]; i++)
		if (!strcasecmp(Genres[i], name))
			return i;
	return 255;
}

/* tab completion for genres. return 1 if update is needed */
static int genre_complete(Input *input)
{
	int matches[256], num_matches;
	int i;

again:
	for (i = num_matches = 0; Genres[i]; i++)
		if (!strncasecmp(input->buf, Genres[i], input->len))
			matches[num_matches++] = i;
	if (num_matches == 0)
		return 0;
	else if (num_matches == 1) {
		strncpy(input->buf, Genres[matches[0]], BUFFER_SIZE-1);
		input->len = strlen(input->buf);
		input->pos = input->fpos = input->len+1;
	}
	else {
		for (i = 0; i < num_matches-1; i++) {
			int a = matches[i], b = matches[i+1];
			if (strlen(Genres[a]) <= input->len || strlen(Genres[b]) <= input->len
					|| Genres[a][input->len] != Genres[b][input->len])
				return 1;
		}
		input->buf[input->len] = Genres[matches[0]][input->len];
		input->len++;
		input->pos++;
		input->fpos++;
		goto again;
	}
	return 1;
}

void
edit_tag(flist *selected)
{
	Input *inputline;
	filename = selected->filename;
	old_active = active;
	old_active->deactivate(old_active);
	active = id3box;
	active->flags |= W_ACTIVE;
	memset(&new_tag, 0, sizeof(new_tag));
	if (read_id3(filename, &id3)) {
		memcpy(new_tag.title, id3.title, 30);
		memcpy(new_tag.artist, id3.artist, 30);
		memcpy(new_tag.album, id3.album, 30);
		memcpy(new_tag.year, id3.year, 4);
		memcpy(new_tag.comment, id3.comment, 30);
	}
	if (id3.genre < 255)
		strncpy(new_tag.genre, Genres[id3.genre], sizeof(new_tag.genre)-1);
	else
		strncpy(new_tag.genre, "Unknown", sizeof(new_tag.genre)-1);
	active->inputline = inputline = (Input *)calloc(1, sizeof(Input));
	inputline->win = active->win;
	inputline->panel = active->panel;
	inputline->x = inputline->y = 0;
	inputline->parse = id3_input;
	inputline->update = update_edit_wrapper;
	inputline->finish = edit_field_finish;
	inputline->plen = 0;
	/* kkkkkkkkkkludgy, do this better */
	inputline->flen = active->width - 26;
	inputline->x = 22+3;
	init_id3_box(active);
	curs_set(1);
	edit_next(inputline);
}

static void edit_next(Input *inputline)
{
	int len = 0;
	char *buf = NULL;
	switch (count) {
		case 0: buf = new_tag.title; len = 30; break;
		case 1: buf = new_tag.artist; len = 30; break;
		case 2: buf = new_tag.album; len = 30; break;
		case 3: buf = new_tag.year; len = 4; break;
		case 4: buf = new_tag.comment; len = 30; break;
		case 5: buf = new_tag.genre; len = sizeof(new_tag.genre); break;
		default: return;
	}

	inputline->y = count+1;
	inputline->anchor = inputline->buf;
	inputline->complete = (count == 5) ? genre_complete : NULL;
	strncpy(inputline->buf, buf, len);
	inputline->buf[len] = '\0';
	inputline->len = (buf[len-1]) ? len : strlen(buf);
	inputline->pos = inputline->fpos = inputline->len+1;
	inputline->update(inputline);
	doupdate();
}

static int edit_field_finish(Input *input)
{
	char *buf;
	switch (count) {
		case 0:
			strncpy(new_tag.title, input->buf, 30);
			buf = new_tag.artist;
			break;
		case 1:
			strncpy(new_tag.artist, input->buf, 30);
			buf = new_tag.album;
			break;
		case 2:
			strncpy(new_tag.album, input->buf, 30);
			buf = new_tag.year;
			break;
		case 3:
			strncpy(new_tag.year, input->buf, 4);
			buf = new_tag.comment;
			break;
		case 4:
			strncpy(new_tag.comment, input->buf, 30);
			buf = new_tag.genre;
			break;
		case 5:
			if (isdigit(*input->buf))
				strncpy(new_tag.genre, Genres[atoi(input->buf)], 19);
			else
				strncpy(new_tag.genre, input->buf, 19);
			buf = new_tag.title;
			break;
		default:
			break;
	}

	my_mvwnaddstr(input->win, input->y, input->x, colors[EDIT_INACTIVE], input->flen, input->buf);
	memset(input->buf, 0, sizeof(input->buf));
	strcpy(input->buf, buf);
	return 1;
}

static void init_id3_box (Window *window)
{
	WINDOW *win = window->win;

	wclear(win);
	window->activate(window);
	wbkgd(win, colors[EDIT_BACK]);
	my_mvwaddstr(win, 1, 13, colors[EDIT_INACTIVE], "Title (30): ");
	if (*new_tag.title)
		my_waddstr(win, colors[EDIT_INACTIVE], new_tag.title);
	my_mvwaddstr(win, 2, 12, colors[EDIT_INACTIVE], "Artist (30): ");
	if (*new_tag.artist)
		my_waddstr(win, colors[EDIT_INACTIVE], new_tag.artist);
	my_mvwaddstr(win, 3, 13, colors[EDIT_INACTIVE], "Album (30): ");
	if (*new_tag.album)
		my_waddstr(win, colors[EDIT_INACTIVE], new_tag.album);
	my_mvwaddstr(win, 4, 14, colors[EDIT_INACTIVE], "Year (30): ");
	if (*new_tag.year)
		my_waddstr(win, colors[EDIT_INACTIVE], new_tag.year);
	my_mvwaddstr(win, 5, 11, colors[EDIT_INACTIVE], "Comment (30): ");
	if (*new_tag.comment)
		my_waddstr(win, colors[EDIT_INACTIVE], new_tag.comment);
	my_mvwaddstr(win, 6, 2, colors[EDIT_INACTIVE], "Genre (0-255 or name): ");
	if (*new_tag.genre)
		my_waddstr(win, colors[EDIT_INACTIVE], new_tag.genre);
}

/* wrapper function for checking arrow keys */
int id3_input (Input *inputline, int c, int alt)
{
	switch (c) {
		case KEY_UP:
			inputline->finish(inputline);
			if (count > 0)
				count--;
			else
				count = 5;
			break;
		case KEY_DOWN:
			inputline->finish(inputline);
			if (count == 5)
				count = 0;
			else
				count++;
			break;
		/* handle our own return key here */
		case KEY_ENTER:
		case '\n':
		case '\r':
			if (!alt) {
				inputline->finish(inputline);
				if (count == 5) {
					finish_edit();
					return 1;
				}
				else
					count++;
			}
			break;
		default:
			do_inputline(inputline, c, alt);
			return 1;
	}
	edit_next(inputline);
	return 1;
}

static int update_edit_wrapper (Input *inputline)
{
	return update_edit (id3box);
}

int update_edit (Window *window)
{
	WINDOW *win = window->win;
	Input *inputline = window->inputline;
	int x = inputline->x, y = inputline->y;	

	update_anchor(inputline);
	my_mvwnaddstr(win, y, x, colors[EDIT_ACTIVE], inputline->flen, inputline->anchor);
	wmove(win, y, x+inputline->fpos-1);
	update_panels();
	doupdate();
	return 1;
}

static int finish_edit (void)
{
	memcpy(id3.title, new_tag.title, 30);
	memcpy(id3.artist, new_tag.artist, 30);
	memcpy(id3.album, new_tag.album, 30);
	memcpy(id3.year, new_tag.year, 4);
	memcpy(id3.comment, new_tag.comment, 30);
	if (isdigit(*new_tag.genre))
		id3.genre = (unsigned char)atoi(new_tag.genre);
	else
		id3.genre = genre_number(new_tag.genre);
	curs_set(0);
	active->deactivate(active);
	active->flags &= ~W_ACTIVE;
	active->update(active);
	free(active->inputline);
	active->inputline = NULL;
	old_active->activate((active = old_active));
	active->flags |= W_ACTIVE;
	active->update(active);
	update_panels();
	doupdate();
	write_id3(filename, &id3);
	count = 0;
	return 1;
}
