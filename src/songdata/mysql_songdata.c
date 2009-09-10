#include "mysql_songdata.h"
#include "songdata.h"
#include "log.h"
#include <string.h>
#include <mysql/mysql.h>
#include <stdio.h>

Config * conf;
MYSQL * mysql;

static void load_songs(songdata *, const char *);
static void load_dirs(songdata *, const char *);

static void load_dirs(songdata * list, const char * dir){

   char sql[2048];
   if(!dir){
     return;
   }
   sprintf(sql,  "SELECT substring(substring(REPLACE(concat(path, dir_path), '%s', ''), 1), 1, locate('/', REPLACE(concat(path, dir_path), '%s', ''), 2) - 1)  as new_path FROM directory_list WHERE concat(path, dir_path) like '%s%%' group by new_path", dir, dir, dir);
   if(!mysql_query(mysql, sql)){
     log_debug_format("MySQL load_dirs Query Executed: %s\n", sql);
     MYSQL_RES * result = mysql_store_result(mysql);
     MYSQL_ROW row;
     while ((row = mysql_fetch_row(result))){
       if(strlen(row[0]) < 2){
         continue;
       }
       songdata_song * item = new_songdata_song();
       item->flags |= F_DIR;
       item->filename = strdup (row[0]);
       item->fullpath = strdup (row[0]);
       item->fullpath = malloc (2048 * sizeof ( char ) );
       sprintf(item->fullpath, "%s%s", dir, row[0]);
       item->path = strdup (dir);
       item->relpath = strdup (dir);
       songdata_add ( list, list->tail, item );
     }
     mysql_free_result(result);
   }else{
     log_debug_format("MySQL load_dirs Query Faild: %s\n", sql);
   }
}

static void load_songs(songdata * list, const char * dir){
  char sql[2048];
//  sprintf(sql,  "SELECT album.name AS album, song.file AS fullpath, concat_ws(' ', artist.prefix, artist.name ) AS artist, genre.name AS genre, song.title AS title, song.track AS track, song.time AS length, catalog.id AS catalog_id FROM song JOIN catalog ON song.catalog = catalog.id JOIN artist ON artist.id = song.artist JOIN genre ON genre.id = song.genre JOIN album ON album.id = song.album WHERE song.file REGEXP '%s/[[:alnum:]]+\\\\.mp3'", dir);
  sprintf(sql,  "SELECT album.name AS album, song.file AS fullpath, concat_ws(' ', artist.prefix, artist.name ) AS artist, genre.name AS genre, song.title AS title, song.track AS track, song.time AS length, catalog.id AS catalog_id FROM song JOIN catalog ON song.catalog = catalog.id JOIN artist ON artist.id = song.artist JOIN genre ON genre.id = song.genre JOIN album ON album.id = song.album WHERE song.file REGEXP '%s[[:print:]]' and SUBSTR( song.file, %d) REGEXP '^[^[.slash.]]+$'", dir, strlen(dir) + 1);
  if(!mysql_query(mysql, sql)){
    log_debug_format("MySQL load_songs Query Executed: %s\n", sql);
    MYSQL_RES * result = mysql_store_result(mysql);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))){
      songdata_song * item = new_songdata_song();
      item->album = strdup(row[0]);
      item->filename = strdup(row[1]); //TODO niet doen??
      item->path = strdup(row[1]);
      item->fullpath = strdup(row[1]);
      item->relpath = strdup(row[1]);
      item->artist = strdup(row[2]);
      item->genre = strdup(row[3]);
      item->title = strdup(row[4]);
      item->has_id3 = 1;
      item->track_id = atoi(row[5]);
      item->length = atoi(row[6]);
      item->catalog_id = atoi(row[7]);
      item->filename = calloc ( strlen ( item->title ) + strlen ( item->artist ) + 4, sizeof ( char ) );
      sprintf ( item->filename, "%s - %s", item->artist, item->title );
      songdata_add ( list, list->tail, item );
    }
    mysql_free_result(result);
  }else{
    log_debug_format("MySQL load_songs Query Faild: %s\n", sql);
  }
}

void mysql_songdata_read_mp3_list_dir ( songdata * list, const char * directory, int append )
{
  char *dir = strdup ( directory );
  if(dir[strlen(dir) - 1] != '/'){
    free(dir);
    dir = (char *)malloc((strlen(directory) + 2) * sizeof(char));
    sprintf(dir, "%s/", directory);
  }

//  int catalog_id = list && list->selected ? list->selected->catalog_id : 1;

  
  log_debug_format("Readdir %s\n", dir);
  
  songdata_clear ( list );

  if ( (( strncmp ( dir, conf->mp3path, strlen ( conf->mp3path )-1 ) ) && ( strcmp ( dir, conf->playlistpath ) ) ) || !dirstack_empty()){
    songdata_song * song;
  
    song = new_songdata_song();
    song->flags |= F_DIR;
    song->filename = strdup ( "../" );
    song->fullpath = strdup ( "../" );
    song->path = strdup ( directory );
    song->relpath = strdup ( directory );
    songdata_add ( list, list->tail, song );
  }
  //laat de dir 
  if(!strcmp(dir, conf->mp3path)){
    //The base dir
    //load cataloges
    if(!mysql_query(mysql, "SELECT id, name, path FROM catalog where enabled = 1\0")){
      log_debug("MySQL load_catalogs Query Executed: SELECT id, name, path FROM catalog where enabled = 1\0\n");
      MYSQL_RES * result = mysql_store_result(mysql);
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(result))){
        songdata_song * item = new_songdata_song();
        item->flags |= F_DIR;
        item->filename = strdup(row[1]);
        item->fullpath = strdup(row[2]);
        item->path = strdup(dir);
        item->relpath = strdup(dir);
        item->catalog_id = atoi(row[0]);
        songdata_add ( list, list->tail, item );
      }
      mysql_free_result(result);
    }else{
      log_debug("MySQL load_catalogs Query Faild: SELECT id, name, path FROM catalog where enabled = 1\0\n");
    }
  }else{
    //doe iets anders
    //laad alle dirs
    //laad alle files
    load_dirs(list, dir);
    load_songs(list, dir);
  }
  //TODO free(dir);
  return;
}

void mysql_songdata_init(Config * thisconf){
  conf = thisconf;
  if (mysql_library_init(0, NULL, NULL)) {
    log_debug("could not initialize MySQL library\n");
    exit(1);
  }

  mysql = mysql_init(mysql);
    
  if (!mysql_real_connect(mysql, "serv.bolkhuis.nl", "mjs", "mjs", "ampache", 0, NULL, 0)){
    log_debug_format("Failed to connect to database: Error: %s\n", (char *)mysql_error(mysql));
    abort();
  }
  log_init();
}

void mysql_songdata_shutdown(){
  mysql_close(mysql);
}