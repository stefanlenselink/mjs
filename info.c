/*
 * most of this code is NOT mine. I don't know where it came from, but it
 * sure as hell is ugly! It _really_ needs some cleanup. Someday...
 */

#include "top.h"
#include "defs.h"
#include "struct.h"
#include "info.h"
#include "misc.h"
#include "window.h"
#include "extern.h"

static u_char	_buffer[32];
static int	_bptr = 0;

static inline int	_fillbfr(int, u_int);
static inline int	gethdr(int, AUDIO_HEADER *);
static inline int	readsync(int);
static inline u_int	_getbits(int);
static inline void	parse_header(AUDIO_HEADER *) ;

static inline int
_fillbfr(int file, u_int size)
{
	_bptr=0;
	if (read(file, _buffer, size) != size)
		return -1;
	return 0;
}
                                        
static inline int
readsync(int file)
{
	_bptr=0;
	_buffer[0]=_buffer[1];
	_buffer[1]=_buffer[2];
	_buffer[2]=_buffer[3];
	if (read(file, &_buffer[3], 1) != 1)
		return -1;
	return 0;
}

static inline u_int
_getbits(int n)
{
	u_int pos,ret_value;

	pos = _bptr >> 3;
	ret_value = _buffer[pos] << 24 |
		_buffer[pos+1] << 16 |
		_buffer[pos+2] << 8 |
		_buffer[pos+3];
		ret_value <<= _bptr & 7;
		ret_value >>= 32 - n;
		_bptr += n;
	return ret_value;
}

/* header and side info parsing stuff */
static inline void
parse_header(AUDIO_HEADER *header) 
{
	header->IDex=_getbits(1);
	header->ID=_getbits(1);
	header->layer=_getbits(2);
	header->protection_bit=_getbits(1);
	header->bitrate_index=_getbits(4);
	header->sampling_frequency=_getbits(2);
	header->padding_bit=_getbits(1);
	header->private_bit=_getbits(1);
	header->mode=_getbits(2);
	header->mode_extension=_getbits(2);
	if (!header->mode) 
		header->mode_extension=0;
	header->copyright=_getbits(1);
	header->original=_getbits(1);
	header->emphasis=_getbits(2);
	header->stereo = (header->mode == 3) ? 1 : 2;
	header->true_layer = 4 - header->layer;
}

static inline int
gethdr(int file, AUDIO_HEADER *header)
{
	int retval, loop = 0;

	if ((retval=_fillbfr(file, 4))!=0) return retval;

	while (_getbits(11) != 0x7ff) {
		if ((retval=readsync(file))!=0) 
			return retval;
		if (loop > 5000)
			return -1;	
		loop++;
	}
	parse_header(header);
	return 1;
}

/* 
 * this routine will clobber existing stats in the file parameter with
 * extreme prejudice. you have been warned.
 */

flist *
mp3_info(char *filename, flist *file, u_int32_t size)
{
	u_int32_t framesize = 0;
	double totalframes = 0;
	short t_bitrate[2][3][15] = {{
		{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
		{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
		{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}
		},{
		{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
		{0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
		{0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}
	}};
	int t_sampling_frequency[2][2][3] = {
		{ /* MPEG 2.5 samplerates */ 
	  		{ 11025, 12000, 8000},
			{ 0, }
		},{ /* MPEG 2.0/1.0 samplerates */
			{ 22050 , 24000 , 16000},
			{ 44100 , 48000 , 32000}
		}
	};

	AUDIO_HEADER header;
	u_int32_t btr = 0;
	int fd = -1;
	ID3tag tmp;
	char *s;

	if ((fd = open(filename, O_RDONLY)) == -1)
		return file;

	memset(&header, 0, sizeof(AUDIO_HEADER));
	if (gethdr(fd, &header) == -1)
		return NULL;

	if (!file)
		file = calloc(1, sizeof(flist));

	btr=t_bitrate[header.ID][3-header.layer][header.bitrate_index];

	framesize = (header.ID ? 144000 : 72000) * btr / (t_sampling_frequency[header.IDex][header.ID][header.sampling_frequency]);
	totalframes = (double)size / (double)framesize;
	file->length = (time_t)(totalframes * (header.ID==0?576.0:1152.0)/(float)t_sampling_frequency[header.IDex][header.ID][header.sampling_frequency]);

	/* Used to do a sizeof() here, but that caused problems under freebsd/egcs? */
	lseek(fd, -128, SEEK_END);
	read(fd, &tmp, sizeof(ID3tag));
	close(fd);
	if (!strncmp("TAG", tmp.tag, 3)) {
		s = tmp.title + 29;
		while (!isalnum(*s))
			*s-- = '\0';
		s = tmp.artist + 29;
		while (!isalnum(*s))
			*s-- = '\0';
//		file->title = (char *)calloc(1, 31);
//		strncpy(file->title, tmp.title, 30);
//		if (strlen(file->title) == 0)
//			strncpy(file->title, filename, 30);
		
		file->artist = (char *)calloc(1, 31);
		strncpy(file->artist, tmp.artist, 30);
		if (strlen(file->artist) == 0)
			strcpy(file->artist, "Unknown");
		file->album = (char *)calloc(1,31);
		strncpy(file->album, tmp.album,30);
	}
	return file;
}
