#ifndef _http_controller_h
#define _http_controller_h

#include "config/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <json.h>
#include <microhttpd.h>

#include "songdata/songdata.h"

void http_controller_init(Config *);
void http_controller_shutdown();
void http_poll();
int http_controller_request(void *, struct MHD_Connection *,
        const char *,
        const char *, const char *,
        const char *,
        size_t *, void **);

int http_controller_headers(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

char* http_get_song_uid(songdata_song *);

void http_post_status(json_value *);

char* http_get_index();
char* http_get_status();
char* http_get_playlist();

#endif /*_http_controller_h */
