#include "top.h"
#include "defs.h"
#include "struct.h"
#include "info.h"
#include "extern.h"
#include "list.h"


/* 	path -> path including trailing slash
	filename -> filename without any slashes
	count -> pointer to int 
	
	mp3path = /pub/mp3/
	
	|*************************fullpath*****************************|
	/pub/mp3/Pop/Cranberries/Bury the hatchet/01 Animal instinct.mp3
	         |*************path*************| |*****filename***|
		 |*******************relpath***************************|
		 |*| |*Artist**| |*****Album****|
		  |
		  \-Genre
	
	|**************abspath**************|

	|***************fullpath*************|
	/pub/mp3/Cranberries/Bury the hatchet/
	         |************path**********| 
			     |***filename***|
		|*relpath**|	     
	|******abspath*****|
*/
__inline__
char *
split_filename(char **s)
{
	char *part;
	char *end;
	int length;
	
	
	
	if (!*s)
		return NULL;
	if ((*s)[0]=='/')
		(*s)++;
	if ((end = strchr (*s, '/')))
		length = end - *s;
	else
		length = strlen (*s);
	part = calloc (length + 1, sizeof (char));
	strncpy (part, *s, length);
	part[length] = '\0';
	*s = end;
	return part;
}

char *
chop_filename(char **directory)
{
	int length;
	char * filename = NULL;
	if (!*directory)
		return NULL;
	
	length = (strrchr (*directory, '/') - *directory);
	if (length < 0)
		return NULL;
	filename = calloc(strlen(*directory) - length, sizeof(char));
	strcpy (filename, *directory + length + 1);
	(*directory)[length] = '\0';
	return filename;
}

char *
resolve_path (const char *path)
{ /* Look whether a symlink is part of the filename, if so return path without symlink */
	
	struct stat st;
	char *newpath = NULL, *oldpath = strdup(path);
	if (lstat (oldpath, &st) != 0)
		return strdup(path); //file does not exist anymore.
	while ((!S_ISLNK (st.st_mode))&&(strlen(oldpath) > conf->basepathlen )){
		*strrchr( oldpath, '/') = '\0';
		lstat (oldpath, &st);
	}
	if (S_ISLNK (st.st_mode)) {

		char tempdir[256];
		int n;
		n = readlink(oldpath, tempdir, sizeof(tempdir));
		tempdir[n]='\0';
		if (tempdir[0]!='.'){ // relative symlinks are not yet supported....
			newpath = calloc(strlen(path)-strlen(oldpath)+n+1, sizeof(char));
			strcpy(newpath, tempdir);
			strcat(newpath, path + strlen(oldpath));
		}
	}
	free(oldpath);
	if (newpath)
		return newpath;
	else
		return strdup(path);
}


char *
strip_track_numbers (const char * filename) 
{ /* Remove tracknumbers from filename */
	char * p;
	if ((filename[0]>='0') & (filename[0]<='9')) {
		if ((p = strchr(filename, ' ')))
			return p + 1;
	} else if (!strncasecmp(filename,"cd",2))
		return filename+7;	
	return filename;
}

flist *
mp3_info (const char *abspath_, const char *filename, const char *playlistname, int count, int append)
{
	flist *ftmp = NULL;
	char *fullpath, *path, *abspath;
	struct stat st;
	unsigned int length;
	int filename_len = strlen (filename);
	
	
	

	if (conf->c_flags & C_USE_GENRE) 
		abspath = resolve_path(abspath_);
	else
		abspath = strdup(abspath_);
		
		

	fullpath = calloc (strlen (abspath) + filename_len + 2, sizeof (char));
	sprintf (fullpath, "%s/%s", abspath, filename);
	if ((length = strlen (conf->mp3path)) == strlen (abspath) + 1)
		path = "\0";
	else
		path = abspath + strlen (conf->mp3path);
	if (path[0]=='/')
		path++;

	if (filename[0]=='.') {
		ftmp = calloc (1, sizeof (flist));
		ftmp->flags |= F_DIR;
		ftmp->filename = strdup ("../");
		ftmp->fullpath = strdup ("../");
		ftmp->path = strdup (path);
		ftmp->relpath = strdup (path);
		return ftmp;
	}

	
	if (!stat (fullpath, &st)) {

		if (S_ISDIR (st.st_mode)) {
			/* File is a directory */
			ftmp = calloc (1, sizeof (flist));
			ftmp->path = calloc (strlen (path) + filename_len + 2, sizeof (char));
			sprintf (ftmp->path, "%s/%s", path, filename);
			ftmp->fullpath = strdup (fullpath);
			ftmp->relpath = strdup (path);
			ftmp->flags |= F_DIR;

			ftmp->filename = calloc (filename_len + 2, sizeof(char));
			strcpy (ftmp->filename, filename);
			if ((count!=0) & (!playlistname))
				strcat (ftmp->filename, ":");
			else 
				strcat(ftmp->filename, "/");
			ftmp->genre = NULL;
			ftmp->album = NULL;
			ftmp->artist = NULL;


		} else if (S_ISREG (st.st_mode)) {
			/* File is a regular file */
			if ((strncasecmp (".mp3", strchr (filename, '\0') - 4, 4)) && (strncasecmp (".mjs", strchr (filename, '\0') - 4, 4)))
				return NULL;

			ftmp = calloc (1, sizeof (flist));
			ftmp->path = strdup (path);
			ftmp->relpath = calloc(strlen(path)+strlen(filename)+2,sizeof(char));
			strcat(ftmp->relpath, path);
			strcat(ftmp->relpath, "/\0");
			strcat(ftmp->relpath, filename);
			ftmp->relpath[strlen(path)+strlen(filename)+1]='\0';
			ftmp->fullpath = strdup (fullpath);
			ftmp->album = NULL;
			ftmp->artist = NULL;

			if (!strncasecmp (".mp3", strchr (filename, '\0') - 4, 4)) {
				/* File is a mp3-file */
				if (conf->c_flags & C_USE_GENRE) {
					ftmp->genre = split_filename(&path);
/* dirty little hack */			if (!strncmp(ftmp->genre, "Electronica",11))
						ftmp->genre = split_filename(&path);
				}
					
				ftmp->artist = split_filename(&path);

				if (playlistname) {
					/* We are reading in a playlist */
					int l;
					ftmp->album = strdup (playlistname);
					filename = strip_track_numbers(filename);
					filename_len = strlen (filename) - 4;
					if ((conf->c_flags & C_TRACK_NUMBERS)||(append == L_NEW)) {
						ftmp->filename = malloc (filename_len + 6);
						l = snprintf (ftmp->filename, 6, "%02.0f ", (float) count);
					} else {
						ftmp->filename = malloc (filename_len+6);
						ftmp->filename[0]='\0';
					}
						
					strncat (ftmp->filename, filename, filename_len);
				} else {
					ftmp->filename = calloc (filename_len - 3, sizeof (char));
					strncpy (ftmp->filename, filename, filename_len - 4);
					ftmp->filename[filename_len - 4] = '\0';

					if (path)
						ftmp->album = strdup (path + 1);
					else
						ftmp->album = NULL;
				}
			} else {
				/* File is a playlist-file */
				ftmp->flags |= F_PLAYLIST;
				ftmp->filename = calloc (strlen (filename) - 2, sizeof (char));
				strncpy (ftmp->filename , filename, filename_len - 4);
				strcat (ftmp->filename, "/\0");
				ftmp->album = NULL;
				ftmp->artist = NULL;

			}
		}
	} else {
		if (!strncasecmp (fullpath, "http", 4)) {
			// web-cast http adres
			ftmp = calloc (1, sizeof (flist));
			ftmp->flags |= F_HTTP;
			ftmp->filename = calloc (strlen (playlistname) + 11, sizeof (char));
			strcpy (ftmp->filename, "WebRadio: \0");
			strcat (ftmp->filename, playlistname);
			ftmp->album = strdup (fullpath);
			ftmp->artist = strdup ("http-stream");
			ftmp->fullpath = strdup (fullpath);
			ftmp->path = strdup ("http-stream");
		}
	}

	free (fullpath);
	free (abspath);
	return ftmp;

}
