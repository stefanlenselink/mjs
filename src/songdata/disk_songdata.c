#include "disk_songdata.h"
#include "songdata.h"
#include "mjs_id3.h"
#include "engine/engine.h"
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

Config * conf;

static songdata_song 	*mp3_info ( const char *, const char *, const char *, int );
static char *  split_filename ( char **s );


static
    char *
    split_filename ( char **s )
{
  char *part;
  char *end;
  int length;
  if ( !*s )
    return NULL;
  if ( ( *s ) [0]=='/' )
    ( *s ) ++;
  if ( ( end = strchr ( *s, '/' ) ) )
    length = end - *s;
  else
    length = strlen ( *s );
  part = calloc ( length + 1, sizeof ( char ) );
  strncpy ( part, *s, length );
  part[length] = '\0';
  *s = end;
  return part;
}

static songdata_song *
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
  if ( ( length = strlen ( conf->mp3path ) ) == strlen ( abspath ) + 1 )
    path = "\0";
  else
  {
		//TODO is dit nog steeds correct?
    path = strdup ( abspath );
    path += strlen ( conf->mp3path );
  }
  if ( path[0]=='/' )
    path++;

  if ( filename[0]=='.' )
  {
    ftmp = new_songdata_song();
    ftmp->flags |= F_DIR;
    ftmp->filename = strdup ( "../" );
    ftmp->fullpath = strdup ( "../" );
    ftmp->path = strdup ( path );
    ftmp->relpath = strdup ( path );
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
      if ( !engine_extention_is_supported(strrchr ( filename, '\0' ) - 3) && strncasecmp ( ".mjs", strchr ( filename, '\0' ) - 4, 4 ))
        return NULL;

      ftmp = new_songdata_song();
      ftmp->path = strdup ( path );
      ftmp->relpath = NULL;
      ftmp->fullpath = strdup ( fullpath );

      if ( strncasecmp ( ".mjs", strchr ( filename, '\0' ) - 4, 4 ) )
      {
// suppored-file
        if ( conf->c_flags & C_USE_GENRE )
          ftmp->genre = split_filename ( &path );

        ftmp->artist = split_filename ( &path );

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
				//id3tag_t tag;
        FILE * file = fopen ( fullpath, "r" );
        if(!file) return NULL;

        mp3info mp3;
        memset ( &mp3,0,sizeof ( mp3info ) );
        mp3.filename = fullpath;
        mp3.file = file;
        if ( get_mp3_info ( &mp3, 1 ) == 0 && mp3.id3_isvalid == 1 )
        {
          id3tag tag = mp3.id3;
          if ( tag.title != NULL && tag.artist != NULL && strcmp ( tag.title, "" ) != 0 && strcmp ( tag.artist, "" ) != 0 && id3_isvalidtag ( tag ) )
          {
            if ( ftmp->title != NULL )
              free ( ftmp->title );
            if ( ftmp->artist != NULL )
              free ( ftmp->artist );
            if ( ftmp->album != NULL )
              free ( ftmp->album );
            if ( ftmp->genre != NULL )
              free ( ftmp->genre );
            if(ftmp->filename != NULL)
              free(ftmp->filename);

            ftmp->title = strdup ( tag.title );
            ftmp->artist = strdup ( tag.artist );
            ftmp->album = strdup ( tag.album );
            ftmp->track_id = ( int ) tag.track;
            ftmp->length = mp3.seconds;
            ftmp->genre = strdup ( id3_findstyle ( tag.genre[0] ) );
            ftmp->has_id3 = 1;
            ftmp->filename = calloc ( strlen ( ftmp->title ) + strlen ( ftmp->artist ) + 4, sizeof ( char ) );
            sprintf ( ftmp->filename, "%s - %s", ftmp->artist, ftmp->title );

          }
        }else{
                  //Other formats Meta info...
          engine_load_meta_info(ftmp);
          if(ftmp->title && ftmp->artist){
            ftmp->filename = calloc ( strlen ( ftmp->title ) + strlen ( ftmp->artist ) + 4, sizeof ( char ) );
            sprintf ( ftmp->filename, "%s - %s", ftmp->artist, ftmp->title );
          }
        }
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
        ftmp->has_id3 = 0;
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

  chdir ( dir );


  if ( ( strncmp ( dir, conf->mp3path, strlen ( conf->mp3path )-1 ) ) && ( strcmp ( dir, conf->playlistpath ) ) )
  {
    /* TODO uitgezet om problemen met nieuwe indeling te voorkomen*/
		/*chdir (conf->mp3path);
    dir = strdup(conf->mp3path);*/
    ftmp = mp3_info ( dir, "../", NULL, 0 );
    songdata_add ( list, list->tail, ftmp );
  }
  else if ( !dirstack_empty() )
  {
    ftmp = mp3_info ( dir, "../", NULL, 0 );
    songdata_add ( list, list->tail, ftmp );
  }


  dptr = opendir ( dir );
  while ( ( dent = readdir ( dptr ) ) )
  {
    if ( *dent->d_name == '.' )
      continue;

    if ( ( ftmp = mp3_info ( dir, dent->d_name, NULL, 0 ) ) )
      songdata_add ( list, list->tail, ftmp );
  }
  closedir ( dptr );
  free ( dir );
  return;
}

void disk_songdata_init(Config * thisconf){
  conf = thisconf;
}