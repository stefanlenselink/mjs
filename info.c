/*
 * most of this code is NOT mine. I don't know where it came from, but it
 * sure as hell is ugly! It _really_ needs some cleanup. Someday...
 */

#include "top.h"
#include "defs.h"
#include "struct.h"
#include "info.h"



flist *
mp3_info(char *filename, flist *file, u_int32_t size)
{
	if (!file)
		file = calloc(1, sizeof(flist));
	file->artist = strdup("Unknown");
	file->album = strdup("Unknown");
	file->length = -1;
	
	return file;

}
	
