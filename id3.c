#include "mms.h"
#include "defs.h"
#include "struct.h"
#include "extern.h"
#include "proto.h"

static ID3tag id3;
static int count = 0;
static int had_tag = 0;
static char genre[20], *filename, *buf, *prompt;
static int len;
static Window *old_active;

static char *Genres[] = {
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
	"Anime", "JPop", "SynthPop", NULL
};

static void edit_next(void);
static int edit_field_finish(Input *);

static void read_id3(char *filename, ID3tag *tag)
{
	
	int fd = open(filename, O_RDONLY);
	
	had_tag = 0;
	if ((fd == -1) || (lseek(fd, -128, SEEK_END) == -1))
		return;
	had_tag = sizeof(ID3tag); /* assume we have a tag */
	memset(tag, 0, sizeof(ID3tag));
	if ((read(fd, tag, 128) == 128) && strncmp(tag->tag, "TAG", 3)) {
		memset(tag, 0, sizeof(ID3tag));
		strncpy(tag->tag, "TAG", 3);
		had_tag = 0; /* i guess we were wrong :) */
	}
	close(fd);
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

void
edit_tag(flist *selected)
{
	filename = selected->filename;
	old_active = active;
	read_id3(filename, &id3);
	if (id3.genre < 255)
		strncpy(genre, Genres[id3.genre], sizeof(genre)-1);
	else
		strncpy(genre, "Unknown", sizeof(genre)-1);
	edit_next();
}

static void edit_next(void)
{
	switch (count) {
		case 0: buf = id3.title; prompt = "Title (30):"; len = 30; break;
		case 1: buf = id3.artist; prompt = "Artist (30):"; len = 30; break;
		case 2: buf = id3.album; prompt = "Album (30):"; len = 30; break;
		case 3: buf = id3.year; prompt = "Year (4):"; len = 4; break;
		case 4: buf = id3.comment; prompt = "Comment (30):"; len = 30; break;
		case 5: buf = genre; prompt = "Genre: (Name or number 0-255):"; len = sizeof(genre); break;
		default: return;
	}

	curs_set(1);
	wbkgd(menubar->win, 0);

	inputline = (Input *)calloc(1, sizeof(Input));
	inputline->win = menubar->win;
	inputline->panel = menubar->panel;
	strncpy(inputline->prompt, prompt, PROMPT_SIZE);
	inputline->plen = strlen(inputline->prompt);
	inputline->flen = 60;
	inputline->anchor = inputline->buf;
	inputline->parse = do_inputline;
	inputline->update = update_menu;
	inputline->finish = edit_field_finish;
	strncpy(inputline->buf, buf, len);
	inputline->buf[len] = '\0';
	inputline->len = (buf[len-1]) ? len : strlen(buf);
	inputline->pos = inputline->fpos = inputline->len+1;
	update_menu(inputline);
	doupdate();
}

static int edit_field_finish(Input *input)
{
	memset(buf, 0, len);
	strncpy(buf, inputline->buf, len);
	free(inputline);
	inputline = NULL;

	wmove(menubar->win, 0, 0);
	wbkgd(menubar->win, MENU_BACK);
	wclrtoeol(menubar->win);

	if (count == 5) {
		if (isdigit(buf[0]))
			id3.genre = (unsigned char)atoi(genre);
		else
			id3.genre = genre_number(genre);
		my_mvwaddstr(menubar->win, 0, 28, MENU_TEXT, version_str);
		curs_set(0);
/* I dont think we need this -- WM
		active->deactivate(active);
		active->flags &= ~W_ACTIVE;
		active->update(active);
		old_active->activate((active = old_active));
		active->flags |= W_ACTIVE;
		active->update(active);
*/
		wmove(menubar->win, 0, 0);
		wbkgd(menubar->win, colors[MENU_BACK]);
		wclrtoeol(menubar->win);
		my_mvwaddstr(menubar->win, 0, 28, colors[MENU_TEXT], version_str);
		update_panels();
		doupdate();
		write_id3(filename, &id3);
		count = 0;
	}
	else {
		count++;
		edit_next();
	}

	return 1;
}
