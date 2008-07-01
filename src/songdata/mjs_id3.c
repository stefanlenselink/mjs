/* id3.c
 *
 * ID3 manipulation tools.
 *
 * Tools for manipulating ID3 tags
*/

/*  id3tool:  An editor for ID3 tags.
 *  Copyright (C) 1999-2002  Christopher Collins
 *
 * This program was authored principly by Christopher Collins (aka
 * Crossfire).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author, Christopher Collins, nor any of his
 *    aliases may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CHRISTOPHER COLLINS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CHRISTOPHER COLLINS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mjs_id3.h"


int layer_tab[4] = {0, 3, 2, 1};

int frequencies[3][4] = {
                            {22050,24000,16000,50000},  /* MPEG 2.0 */
                            {44100,48000,32000,50000},  /* MPEG 1.0 */
                            {11025,12000,8000,50000}    /* MPEG 2.5 */
                        };

int bitrate[2][3][14] = {
                            { /* MPEG 2.0 */
                                {32,48,56,64,80,96,112,128,144,160,176,192,224,256},  /* layer 1 */
                                {8,16,24,32,40,48,56,64,80,96,112,128,144,160},       /* layer 2 */
                                {8,16,24,32,40,48,56,64,80,96,112,128,144,160}        /* layer 3 */
                            },

                            { /* MPEG 1.0 */
                                {32,64,96,128,160,192,224,256,288,320,352,384,416,448}, /* layer 1 */
                                {32,48,56,64,80,96,112,128,160,192,224,256,320,384},    /* layer 2 */
                                {32,40,48,56,64,80,96,112,128,160,192,224,256,320}      /* layer 3 */
                            }
                        };

int frame_size_index[] = {24000, 72000, 72000};


char *mode_text[] = {
                        "stereo", "joint stereo", "dual channel", "mono"
                    };

char *emphasis_text[] = {
                            "none", "50/15 microsecs", "reserved", "CCITT J 17"
                        };


struct style_s	id3_styles[] =
    {
	    {0x00, "Blues"},
	    {0x01, "Classic Rock"},
	    {0x02, "Country"},
	    {0x03, "Dance"},
	    {0x04, "Disco"},
	    {0x05, "Funk"},
	    {0x06, "Grunge"},
	    {0x07, "Hip-Hop"},
	    {0x08, "Jazz"},
	    {0x09, "Metal"},
	    {0x0A, "New Age"},
	    {0x0B, "Oldies"},
	    {0x0C, "Other"},
	    {0x0D, "Pop"},
	    {0x0E, "R&B"},
	    {0x0F, "Rap"},
	    {0x10, "Reggae"},
	    {0x11, "Rock"},
	    {0x12, "Techno"},
	    {0x13, "Industrial"},
	    {0x14, "Alternative"},
	    {0x15, "Ska"},
	    {0x16, "Death Metal"},
	    {0x17, "Pranks"},
	    {0x18, "Soundtrack"},
	    {0x19, "Euro-Techno"},
	    {0x1A, "Ambient"},
	    {0x1B, "Trip-Hop"},
	    {0x1C, "Vocal"},
	    {0x1D, "Jazz+Funk"},
	    {0x1E, "Fusion"},
	    {0x1F, "Trance"},
	    {0x20, "Classical"},
	    {0x21, "Instrumental"},
	    {0x22, "Acid"},
	    {0x23, "House"},
	    {0x24, "Game"},
	    {0x25, "Sound Clip"},
	    {0x26, "Gospel"},
	    {0x27, "Noise"},
	    {0x28, "Alt. Rock"},
	    {0x29, "Bass"},
	    {0x2A, "Soul"},
	    {0x2B, "Punk"},
	    {0x2C, "Space"},
	    {0x2D, "Meditative"},
	    {0x2E, "Instrumental Pop"},
	    {0x2F, "Instrumental Rock"},
	    {0x30, "Ethnic"},
	    {0x31, "Gothic"},
	    {0x32, "Darkwave"},
	    {0x33, "Techno-Industrial"},
	    {0x34, "Electronic"},
	    {0x35, "Pop-Folk"},
	    {0x36, "Eurodance"},
	    {0x37, "Dream"},
	    {0x38, "Southern Rock"},
	    {0x39, "Comedy"},
	    {0x3A, "Cult"},
	    {0x3B, "Gangsta"},
	    {0x3C, "Top 40"},
	    {0x3D, "Christian Rap"},
	    {0x3E, "Pop/Funk"},
	    {0x3F, "Jungle"},
	    {0x40, "Native US"},
	    {0x41, "Cabaret"},
	    {0x42, "New Wave"},
	    {0x43, "Psychadelic"},
	    {0x44, "Rave"},
	    {0x45, "Showtunes"},
	    {0x46, "Trailer"},
	    {0x47, "Lo-Fi"},
	    {0x48, "Tribal"},
	    {0x49, "Acid Punk"},
	    {0x4A, "Acid Jazz"},
	    {0x4B, "Polka"},
	    {0x4C, "Retro"},
	    {0x4D, "Musical"},
	    {0x4E, "Rock & Roll"},
	    {0x4F, "Hard Rock"},
	    {0x50, "Folk"},
	    {0x51, "Folk-Rock"},
	    {0x52, "National Folk"},
	    {0x53, "Swing"},
	    {0x54, "Fast Fusion"},
	    {0x55, "Bebob"},
	    {0x56, "Latin"},
	    {0x57, "Revival"},
	    {0x58, "Celtic"},
	    {0x59, "Bluegrass"},
	    {0x5A, "Avantgarde"},
	    {0x5B, "Gothic Rock"},
	    {0x5C, "Progressive Rock"},
	    {0x5D, "Psychedelic Rock"},
	    {0x5E, "Symphonic Rock"},
	    {0x5F, "Slow Rock"},
	    {0x60, "Big Band"},
	    {0x61, "Chorus"},
	    {0x62, "Easy Listening"},
	    {0x63, "Acoustic"},
	    {0x64, "Humour"},
	    {0x65, "Speech"},
	    {0x66, "Chanson"},
	    {0x67, "Opera"},
	    {0x68, "Chamber Music"},
	    {0x69, "Sonata"},
	    {0x6A, "Symphony"},
	    {0x6B, "Booty Bass"},
	    {0x6C, "Primus"},
	    {0x6D, "Porn Groove"},
	    {0x6E, "Satire"},
	    {0x6F, "Slow Jam"},
	    {0x70, "Club"},
	    {0x71, "Tango"},
	    {0x72, "Samba"},
	    {0x73, "Folklore"},
	    {0x74, "Ballad"},
	    {0x75, "Power Ballad"},
	    {0x76, "Rhytmic Soul"},
	    {0x77, "Freestyle"},
	    {0x78, "Duet"},
	    {0x79, "Punk Rock"},
	    {0x7A, "Drum Solo"},
	    {0x7B, "Acapella"},
	    {0x7C, "Euro-House"},
	    {0x7D, "Dance Hall"},
	    {0x7E, "Goa"},
	    {0x7F, "Drum & Bass"},
	    {0x80, "Club-House"},
	    {0x81, "Hardcore"},
	    {0x82, "Terror"},
	    {0x83, "Indie"},
	    {0x84, "BritPop"},
	    {0x85, "Negerpunk"},
	    {0x86, "Polsk Punk"},
	    {0x87, "Beat"},
	    {0x88, "Christian Gangsta Rap"},
	    {0x89, "Heavy Metal"},
	    {0x8A, "Black Metal"},
	    {0x8B, "Crossover"},
	    {0x8C, "Contemporary Christian"},
	    {0x8D, "Christian Rock"},
	    {0x8E, "Merengue"},
	    {0x8F, "Salsa"},
	    {0x90, "Trash Metal"},
	    {0xFF, NULL}
    };

static int isvalid ( const char *, int );
static char *unpad ( char * );
static int get_next_header ( mp3info * );
static int get_first_header ( mp3info *, long );
static int get_header ( FILE *,mp3header * );
static int frame_length ( mp3header * );
static int sameConstant ( mp3header *, mp3header * );
static int get_id3 ( mp3info *, int withBHID);
static void searchBHID(mp3info *);

static int isvalid ( const char * field, int checkSpaces )
{
	unsigned i;
	unsigned spacecounter = 0;
	for ( i = 0; i < strlen ( field ); i++ )
		if ( !isprint ( field[i] ) )
			return 0;
		else if ( isblank ( field[i] ) )
			spacecounter++;
	if ( checkSpaces && spacecounter == strlen ( field ) )
		return 0;
	return 1;
}

/* Remove trailing whitespace from the end of a string */
static char *unpad ( char *string )
{
	char *pos=string+strlen ( string )-1;
	while ( isspace ( pos[0] ) ) ( pos-- ) [0]=0;
	return string;
}

static int get_first_header ( mp3info *mp3, long startpos )
{
	int k, l=0,c;
	mp3header h, h2;
	long valid_start=0;

	fseek ( mp3->file,startpos,SEEK_SET );
	while ( 1 )
	{
		while ( ( c=fgetc ( mp3->file ) ) != 255 && ( c != EOF ) );
		if ( c == 255 )
		{
			ungetc ( c,mp3->file );
			valid_start=ftell ( mp3->file );
			if ( ( l=get_header ( mp3->file,&h ) ) )
			{
				fseek ( mp3->file,l-FRAME_HEADER_SIZE,SEEK_CUR );
				for ( k=1; ( k < MIN_CONSEC_GOOD_FRAMES ) && ( mp3->datasize-ftell ( mp3->file ) >= FRAME_HEADER_SIZE ); k++ )
				{
					if ( ! ( l=get_header ( mp3->file,&h2 ) ) ) break;
					if ( !sameConstant ( &h,&h2 ) ) break;
					fseek ( mp3->file,l-FRAME_HEADER_SIZE,SEEK_CUR );
				}
				if ( k == MIN_CONSEC_GOOD_FRAMES )
				{
					fseek ( mp3->file,valid_start,SEEK_SET );
					memcpy ( & ( mp3->header ),&h2,sizeof ( mp3header ) );
					mp3->header_isvalid=1;
					return 1;
				}
			}
		}
		else
		{
			return 0;
		}
	}

	return 0;
}

/* get_next_header() - read header at current position or look for
   the next valid header if there isn't one at the current position
*/
static int get_next_header ( mp3info *mp3 )
{
	int l=0,c,skip_bytes=0;
	mp3header h;

	while ( 1 )
	{
		while ( ( c=fgetc ( mp3->file ) ) != 255 && ( ftell ( mp3->file ) < mp3->datasize ) ) skip_bytes++;
		if ( c == 255 )
		{
			ungetc ( c,mp3->file );
			if ( ( l=get_header ( mp3->file,&h ) ) )
			{
				if ( skip_bytes ) mp3->badframes++;
				fseek ( mp3->file,l-FRAME_HEADER_SIZE,SEEK_CUR );
				return 15-h.bitrate;
			}
			else
			{
				skip_bytes += FRAME_HEADER_SIZE;
			}
		}
		else
		{
			if ( skip_bytes ) mp3->badframes++;
			return 0;
		}
	}
}
/* Get next MP3 frame header.
   Return codes:
   positive value = Frame Length of this header
   0 = No, we did not retrieve a valid frame header
*/

static int get_header ( FILE *file,mp3header *header )
{
	unsigned char buffer[FRAME_HEADER_SIZE];
	int fl;

	if ( fread ( &buffer,FRAME_HEADER_SIZE,1,file ) <1 )
	{
		header->sync=0;
		return 0;
	}
	header->sync= ( ( ( int ) buffer[0]<<4 ) | ( ( int ) ( buffer[1]&0xE0 ) >>4 ) );
	if ( buffer[1] & 0x10 ) header->version= ( buffer[1] >> 3 ) & 1;
	else header->version=2;
	header->layer= ( buffer[1] >> 1 ) & 3;
	if ( ( header->sync != 0xFFE ) || ( header->layer != 1 ) )
	{
		header->sync=0;
		return 0;
	}
	header->crc=buffer[1] & 1;
	header->bitrate= ( buffer[2] >> 4 ) & 0x0F;
	header->freq= ( buffer[2] >> 2 ) & 0x3;
	header->padding= ( buffer[2] >>1 ) & 0x1;
	header->extension= ( buffer[2] ) & 0x1;
	header->mode= ( buffer[3] >> 6 ) & 0x3;
	header->mode_extension= ( buffer[3] >> 4 ) & 0x3;
	header->copyright= ( buffer[3] >> 3 ) & 0x1;
	header->original= ( buffer[3] >> 2 ) & 0x1;
	header->emphasis= ( buffer[3] ) & 0x3;

	return ( ( fl=frame_length ( header ) ) >= MIN_FRAME_SIZE ? fl : 0 );
}

static int frame_length ( mp3header *header )
{
	return header->sync == 0xFFE ?
	       ( frame_size_index[3-header->layer]* ( ( header->version&1 ) +1 ) *
	         header_bitrate ( header ) /header_frequency ( header ) ) +
	       header->padding : 1;
}

int header_layer(mp3header *h) {return layer_tab[h->layer];}

int header_bitrate ( mp3header *h )
{
	if ( !h->bitrate )
		return 0;
	return bitrate[h->version & 1][3-h->layer][h->bitrate-1];
}

int header_frequency ( mp3header *h )
{
	return frequencies[h->version][h->freq];
}

static int sameConstant ( mp3header *h1, mp3header *h2 )
{
	if ( ( * ( uint* ) h1 ) == ( * ( uint* ) h2 ) ) return 1;

	if ( ( h1->version       == h2->version ) &&
	        ( h1->layer         == h2->layer ) &&
	        ( h1->crc           == h2->crc ) &&
	        ( h1->freq          == h2->freq ) &&
	        ( h1->mode          == h2->mode ) &&
	        ( h1->copyright     == h2->copyright ) &&
	        ( h1->original      == h2->original ) &&
	        ( h1->emphasis      == h2->emphasis ) )
		return 1;
	else return 0;
}

static int get_id3 ( mp3info *mp3, int withBHID)
{
	int retcode=0;
	char fbuf[4];

	if ( mp3->datasize >= 128 )
	{
		if ( fseek ( mp3->file, -128, SEEK_END ) )
		{
			fprintf ( stderr,"ERROR: Couldn't read last 128 bytes of %s!!\n",mp3->filename );
			retcode |= 4;
		}
		else
		{
			fread ( fbuf,1,3,mp3->file ); fbuf[3] = '\0';
			mp3->id3.genre[0]=255;


			if ( !strcmp ( ( const char * ) "TAG", ( const char * ) fbuf ) )
			{
				mp3->id3.bhid = -1;
				mp3->bhid_isvalid = 0;
				if(withBHID)
				{
					fseek ( mp3->file, -136, SEEK_END );
					searchBHID(mp3);
				}
				fseek ( mp3->file, -125, SEEK_END );
				mp3->datasize -= 128;
				mp3->id3_isvalid=1;
				//fseek(mp3->file, -125, SEEK_END);
				fread ( mp3->id3.title,1,30,mp3->file ); mp3->id3.title[30] = '\0';
				fread ( mp3->id3.artist,1,30,mp3->file ); mp3->id3.artist[30] = '\0';
				fread ( mp3->id3.album,1,30,mp3->file ); mp3->id3.album[30] = '\0';
				fread ( mp3->id3.year,1,4,mp3->file ); mp3->id3.year[4] = '\0';
				fread ( mp3->id3.comment,1,30,mp3->file ); mp3->id3.comment[30] = '\0';
				if ( mp3->id3.comment[28] == '\0' )
				{
					mp3->id3.track[0] = mp3->id3.comment[29];
				}
				else
				{
					mp3->id3.track[0] = 0;
				}
				// Moet hier niet nog een track-nummer
				fread ( mp3->id3.genre,1,1,mp3->file );
				unpad ( mp3->id3.title );
				unpad ( mp3->id3.artist );
				unpad ( mp3->id3.album );
				unpad ( mp3->id3.year );
				unpad ( mp3->id3.comment );
			}
			else if(withBHID)
			{
				fseek ( mp3->file, -8, SEEK_END );
				searchBHID(mp3);
			}
			
		}
	}
	return retcode;

}
static void searchBHID(mp3info * mp3)
{
	int bytescounter = 0;
	int intBuff;
	char Buff[4];
	mp3->datasize -= 8;
	fread ( mp3->id3.bhtag,1,4,mp3->file );
	mp3->id3.bhtag[4] = '\0';
	if ( !strcmp ( ( const char * ) "BHID",mp3->id3.bhtag ) )
	{
		mp3->bhid_isvalid = 1;
		fread ( Buff,1,4,mp3->file );
		intBuff = Buff[0] << 24;
		intBuff |= Buff[1] << 16;
		intBuff |= Buff[2] << 8;
		intBuff |= Buff[3];
		mp3->id3.bhid = (int) intBuff;
		return;
	}
	while(strcmp ( ( const char * ) "BHID",mp3->id3.bhtag ) && bytescounter < 1024) {
		fseek( mp3->file, -5, SEEK_CUR );
		fread ( mp3->id3.bhtag,1,4,mp3->file );
		bytescounter++;
	}
	if(bytescounter < 1024)
	{
		mp3->id3.bhtag[4] = '\0';
		fread ( Buff,1,4,mp3->file );
		intBuff = Buff[0] << 24;
		intBuff |= Buff[1] << 16;
		intBuff |= Buff[2] << 8;
		intBuff |= Buff[3];
		mp3->id3.bhid = (int) intBuff;
		mp3->bhid_isvalid = 1;
	}
}
extern int get_mp3_info ( mp3info *mp3, int id3Only )
{
	int had_error = 0;
	int frame_type[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	float seconds=0,total_rate=0;
	int frames=0,frame_types=0,frames_so_far=0;
	int l,vbr_median=-1;
	int bitrate,lastrate;
	int counter=0;
	mp3header header;
	struct stat filestat;
	off_t sample_pos,data_start=0;


	stat ( mp3->filename,&filestat );
	mp3->datasize = filestat.st_size;
	get_id3 ( mp3, !id3Only );
	if ( id3Only )
		return had_error;
	if ( get_first_header ( mp3,0L ) )
	{
		data_start = ftell ( mp3->file );
		while ( ( bitrate = get_next_header ( mp3 ) ) )
		{
			frame_type[15-bitrate]++;
			frames++;
		}
		memcpy ( &header,& ( mp3->header ),sizeof ( mp3header ) );
		for ( counter=0;counter<15;counter++ )
		{
			if ( frame_type[counter] )
			{
				frame_types++;
				header.bitrate=counter;
				frames_so_far += frame_type[counter];
				seconds += ( float ) ( frame_length ( &header ) *frame_type[counter] ) /
				           ( float ) ( header_bitrate ( &header ) *125 );
				total_rate += ( float ) ( ( header_bitrate ( &header ) ) *frame_type[counter] );
				if ( ( vbr_median == -1 ) && ( frames_so_far >= frames/2 ) )
					vbr_median=counter;
			}
		}
		mp3->seconds= ( int ) ( seconds+0.5 );
		mp3->header.bitrate=vbr_median;
		mp3->vbr_average=total_rate/ ( float ) frames;
		mp3->frames=frames;
		if ( frame_types > 1 )
		{
			mp3->vbr=1;
		}
	}
	return had_error;
}

// extern int id3_readtag(FILE *fin, id3tag_t *id3tag) {
// 	int	n;
// 	if (NULL == fin || NULL == id3tag) {
// 		return (-1);
// 	}
// 	fseek (fin, 0, SEEK_END);
// 	fseek (fin, ftell(fin)-128, SEEK_SET);
// 	n = fread (id3tag, 128, 1, fin);
// 	if (1 != n || strncmp(id3tag->magic, "TAG", 3)) {
// 		return (1);
// 	}
// 	return (0);
// }

extern int id3_isvalidtag ( id3tag tag )
{
	if ( isvalid ( tag.title, 1 ) && isvalid ( tag.artist, 1 ) && isvalid ( tag.album, 0 ) )
		return 1;
	return 0;
}

extern char * id3_findstyle ( int styleid )
{
	int	ctr;
	for ( ctr = 0; id3_styles[ctr].name != NULL; ctr++ )
	{
		if ( id3_styles[ctr].styleid == styleid )
		{
			return ( id3_styles[ctr].name );
		}
	}
	return "Unknown Style";
}

