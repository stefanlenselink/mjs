/* id3.h
 *
 * ID3 format
 *
 * Part of id3tool
 *
 * Copyright (C) 1999-2002, Christopher Collins
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

#ifndef _ID3_H
#define _ID3_H

// typedef struct id3tag_s {
// 	char		magic[3];
// 	char		songname[30];
// 	char		artist[30];
// 	char		album[30];
// 	char		year[4];
// 	union {
// 		struct {
// 			char		note[30];
// 		} v10;
// 		struct {
// 			char		note[28];
// 			char		marker;
// 			unsigned char	track;
// 		} v11;
// 	} note;
// 	unsigned char	style;
// } id3tag_t;
//

#define MIN_CONSEC_GOOD_FRAMES 4
#define FRAME_HEADER_SIZE 4
#define MIN_FRAME_SIZE 21
#define NUM_SAMPLES 4

#include <stdio.h>

typedef struct
{
	unsigned long   sync;
	unsigned int    version;
	unsigned int    layer;
	unsigned int    crc;
	unsigned int    bitrate;
	unsigned int    freq;
	unsigned int    padding;
	unsigned int    extension;
	unsigned int    mode;
	unsigned int    mode_extension;
	unsigned int    copyright;
	unsigned int    original;
	unsigned int    emphasis;
}
mp3header;

typedef struct
{
	char bhtag[5];
	int  bhid;
	char title[31];
	char artist[31];
	char album[31];
	char year[5];
	char comment[31];
	unsigned char track[1];
	unsigned char genre[1];
}
id3tag;

typedef struct
{
	char *filename;
	FILE *file;
	off_t datasize;
	int header_isvalid;
	mp3header header;
	int id3_isvalid;
	int bhid_isvalid;
	id3tag id3;
	int vbr;
	float vbr_average;
	int seconds;
	int frames;
	int badframes;
}
mp3info;

struct style_s
{
	unsigned char	styleid;
	char		*name;
};

// extern struct style_s	id3_styles[];
int header_bitrate ( mp3header * );
int header_frequency ( mp3header * );
int header_layer(mp3header *h);
// extern int	id3_readtag (FILE *fin, id3tag_t *id3tag);
extern int	id3_isvalidtag ( id3tag tag );
extern char	*id3_findstyle ( int styleid );
extern int get_mp3_info ( mp3info *mp3, int id3Only );

#endif /* #ifndef _ID3_H */
