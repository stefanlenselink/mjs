#ifndef _lastfm_h
#define _lastfm_h

#include <sys/time.h>
#include <time.h>

typedef struct {
  int succes;
  int baned;
  int badauth;
  int badtime;
  int interval;
  struct timeval * last_submit;
  char * username;
  char * passwd;
  
  //Only When OK
  char * session_id;
  char * submit_url;
  char * now_playing_url;
  
  //Only when failure and not baduser
  char * failure;
} LastFMHandshake;

void initLastFM(LastFMHandshake * handshake, char * username, char * passwd);

void submitLastFM(LastFMHandshake * handshake, char * name, char * title, char * album, int length, int trackid);

void currentPlayingLastFM(LastFMHandshake * handshake, char * name, char * title, char * album, int length, int trackid);
    
void deleteLastFMHandshake(LastFMHandshake * handshake, int full_delete);

extern LastFMHandshake * lastFMHandshake;

#endif /* _lastfm_h */
