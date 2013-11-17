#include "disk_songdata.h"
#include "songdata.h"
#include "engine/engine.h"
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

Config * conf;

char *
split_filename(char * filename) {
	char *part;
	char *end;
	char * s;
	int length;
	if (!filename) {
		return NULL;
	}
	if (filename[0] == '/') {
		s = strdup(filename + 1);
	} else {
		s = strdup(filename);
	}
	if ((end = strchr(s, '/'))) {
		length = end - s;
	} else {
		length = strlen(s);
	}
	part = calloc(length + 1, sizeof(char));
	strncpy(part, s, length);
	part[length] = '\0';
	free(s);
	return part;
}

songdata_song *
    mp3_info ( const char *abspath, const char *filename, const char *playlistname, int count )
{
  songdata_song *ftmp = NULL;
  char *fullpath, *path, newpath[256];
  struct stat st;
  unsigned int length;
  int filename_len = strlen ( filename );

	// look whether a symlink is part of the filename
  if ( conf->c_flags & C_USE_GENRE )
  {
    struct stat st;
    char *sympath;
    sympath = strdup ( abspath );
    lstat ( sympath, &st );

    /* FIXME --> (strlen(sympath)>9 ) */

    while ( ( !S_ISLNK ( st.st_mode ) ) && ( strlen ( sympath ) >9 ) )
    {
      *strrchr ( sympath, '/' ) = '\0';
      lstat ( sympath, &st );
    }
    if ( S_ISLNK ( st.st_mode ) )
    {

      char tempdir[256];
      int n;
      n = readlink ( sympath, tempdir, sizeof ( tempdir ) );
      tempdir[n]='\0';
      strncpy ( newpath, tempdir, sizeof ( newpath ) );
      strncat ( newpath, abspath + strlen ( sympath ), sizeof ( newpath ) - strlen ( newpath ) );
      abspath = newpath;
    }
    free ( sympath );
  }




  fullpath = calloc ( strlen ( abspath ) + filename_len + 2, sizeof ( char ) );
  sprintf ( fullpath, "%s/%s", abspath, filename );
  int jmp = 0;
  if ( ( length = strlen ( conf->mp3path ) ) == strlen ( abspath ) + 1 ){
    path = strdup("\0");
  }
  else
  {
		jmp = strlen ( conf->mp3path );
	  if ( abspath[jmp]=='/' ){
		  jmp++;
	  }

	  path = strdup(abspath + jmp);
  }
  if ( filename[0]=='.' )
  {
    ftmp = new_songdata_song();
    ftmp->flags |= F_DIR;
    ftmp->filename = strdup ( "../" );
    ftmp->fullpath = strdup ( "../" );
    ftmp->path = strdup ( path );
    ftmp->relpath = strdup ( path );
    free(fullpath);
    free(path);
    return ftmp;
  }


  if ( !stat ( fullpath, &st ) )
  {

    if ( S_ISDIR ( st.st_mode ) )
    {
//directory
      ftmp = new_songdata_song();
      ftmp->path = calloc ( strlen ( path ) + filename_len + 2, sizeof ( char ) );
      sprintf ( ftmp->path, "%s/%s", path, filename );
      ftmp->fullpath = strdup ( fullpath );
      ftmp->relpath = strdup ( path );
      ftmp->flags |= F_DIR;

      ftmp->filename = calloc ( filename_len + 2, sizeof ( char ) );
      strcpy ( ftmp->filename, filename );
      if ( ( count!=0 ) & ( !playlistname ) )
        strcat ( ftmp->filename, ":" );
      else
        strcat ( ftmp->filename, "/" );
      ftmp->title = strdup ( ftmp->filename );
    }
    else if ( S_ISREG ( st.st_mode ) )
    {
      if ( !engine_extention_is_supported(strrchr ( filename, '\0' ) - 3) && strncasecmp ( ".mjs", strchr ( filename, '\0' ) - 4, 4 )){
    	  free(fullpath);
    	  free(path);
        return NULL;
      }

      ftmp = new_songdata_song();
      ftmp->path = strdup ( path );
      ftmp->relpath = NULL;
      ftmp->fullpath = strdup ( fullpath );

      if ( strncasecmp ( ".mjs", strchr ( filename, '\0' ) - 4, 4 ) )
      {
// suppored-file
        if ( conf->c_flags & C_USE_GENRE )
          ftmp->genre = split_filename ( path );

        ftmp->artist = split_filename ( path );

        if ( playlistname )
        {
          int l;
          ftmp->album = strdup ( playlistname );
					// get rid of old tracknumber add new tracknumber
          if ( ( filename[0] >= '0' ) & ( filename[0] <= '9' ) )
            filename = filename + 3;
          filename_len = strlen ( filename ) - 4;
          ftmp->filename = malloc ( filename_len + 6 );
          memset(ftmp->filename, 0, filename_len + 6);
          l = snprintf ( ftmp->filename, 6, "%02.0f ", ( float ) count );
          strncat ( ftmp->filename, filename, filename_len );
        }
        else
        {
          ftmp->filename = malloc (sizeof ( char )  * (filename_len - 3));
          strncpy ( ftmp->filename, filename, filename_len - 4 );
          ftmp->filename[filename_len - 4] = '\0';

          if ( path )
            ftmp->album = strdup ( path + 1 );
          else
            ftmp->album = NULL;
        }
        ftmp->title = strdup ( ftmp->filename );
        FILE * file = fopen ( fullpath, "r" );
        if(!file){
        	free(fullpath);
        	free(path);
        	return NULL;
        }
        engine_load_meta_info(ftmp);
        if(ftmp->title && ftmp->artist){
          if(ftmp->filename != NULL){
            free(ftmp->filename);
          }
          ftmp->filename = calloc ( strlen ( ftmp->title ) + strlen ( ftmp->artist ) + 4, sizeof ( char ) );
          sprintf ( ftmp->filename, "%s - %s", ftmp->artist, ftmp->title );
        }
        gui_update_filelist();
        fclose ( file );
      }
      else
      {
// playlist-file
        ftmp->flags |= F_PLAYLIST;
        ftmp->filename = calloc ( strlen ( filename ) - 2, sizeof ( char ) );
        strncpy ( ftmp->filename , filename, filename_len - 4 );
        strcat ( ftmp->filename, "/\0" );
        ftmp->title = strdup ( ftmp->filename );
        ftmp->album = NULL;
        ftmp->artist = NULL;
      }
    }
  }
  else
  {
    if ( !strncasecmp ( fullpath, "http", 4 ) )
    {
			// web-cast http adres
      ftmp = new_songdata_song();
      ftmp->flags |= F_HTTP;
      ftmp->filename = calloc ( strlen ( playlistname ) + 11, sizeof ( char ) );
      strcpy ( ftmp->filename, "WebRadio: \0" );
      strcat ( ftmp->filename, playlistname );
      ftmp->title = strdup ( ftmp->filename );
      ftmp->album = strdup ( fullpath );
      ftmp->artist = strdup ( "http-stream" );
      ftmp->fullpath = strdup ( fullpath );
      ftmp->path = strdup ( "http-stream" );
    }
  }
  free ( fullpath );
  free(path);
  return ftmp;

}


void disk_songdata_read_mp3_list_dir ( songdata * list, const char * directory, int append )
{
  char *dir = NULL;
  DIR *dptr = NULL;
  struct dirent *dent;
  songdata_song *ftmp = NULL;

  dir = strdup ( directory );

  songdata_clear ( list );

  //chdir ( dir ); TODO check met kees of dit klopt


  if ( ( strncmp ( dir, conf->mp3path, strlen ( conf->mp3path )-1 ) ) && ( strcmp ( dir, conf->playlistpath ) ) )
  {
    /* TODO uitgezet om problemen met nieuwe indeling te voorkomen*/
		/*chdir (conf->mp3path);
    dir = strdup(conf->mp3path);*/
    ftmp = mp3_info ( dir, "../", NULL, 0 );
    songdata_add_ordered ( list, ftmp );
  }
  else if ( !dirstack_empty() )
  {
    ftmp = mp3_info ( dir, "../", NULL, 0 );
    songdata_add_ordered ( list, ftmp );
  }


  dptr = opendir ( dir );
  while ( ( dent = readdir ( dptr ) ) )
  {
    if ( *dent->d_name == '.' || dent->d_name == NULL)
      continue;

    if ( ( ftmp = mp3_info ( dir, dent->d_name, NULL, 0 ) ) ){
      songdata_add_ordered ( list, ftmp );
	}
  }
  closedir ( dptr );
  free ( dir );
  return;
}

void disk_songdata_init(Config * thisconf){
  conf = thisconf;
}
void disk_songdata_shutdown(){
}
