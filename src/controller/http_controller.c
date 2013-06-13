/***************************************************************************
 *   Copyright (C) 2012 by Max Maton                                       *
 *   max@maton.info                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "http_controller.h"
#include "controller/controller.h"

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

#include <controller/json.h>
#include <songdata/disk_songdata.h>
#include <microhttpd.h>

struct MHD_Daemon *http_daemon;

songdata_song * http_song_by_uid(char *);

void http_controller_init(Config * cnf) {
    http_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 8080, NULL, NULL, &http_controller_request, NULL, MHD_OPTION_END);
}

void http_controller_shutdown(void) {
	MHD_stop_daemon(http_daemon);
}

void http_poll(void) {
}

int http_controller_request(void *cls, struct MHD_Connection *connection,
        const char *url,
        const char *method, const char *version,
        const char *upload_data,
        size_t *upload_data_size, void **con_cls)
{

    char *page = NULL;

    if(!strcmp(method, "GET"))
    {
        if(!strcmp(url, "/"))
            page = http_get_index();
	else if (!strcmp(url, "/playlist"))
	    page = http_get_playlist();
	else if (!strcmp(url, "/status"))
	    page = http_get_status();
	else if (!strcmp(url, "/current"))
	    page = http_get_current();
	else if (!strncmp(url, "/playlist/", 10))
	{
	    char *request = strdup(url);
	    page = http_get_playlist_item(request);
	    free(request);
	}

    } else if(!strcmp(method, "POST"))
    {
        //Calculate content_length
        int content_length = 0;
        MHD_get_connection_values(connection, MHD_HEADER_KIND, http_controller_headers, &content_length);
        
        //We need more post data
        if(content_length > *upload_data_size)
            return MHD_YES;
	
	//Create string from upload_data
	char *strdata = malloc(content_length + 1);
	strncpy(strdata, upload_data, content_length);
	strdata[content_length] = 0;

        //Parse JSON
        json_value *data = json_parse(strdata);
	free(strdata);
	strdata = NULL;

	//Ignore malformed JSON
	if(data)
	{
            //Execute commands
            if(!strcmp(url, "/status"))
                page = http_post_status(data);
	    else if(!strcmp(url, "/current"))
		page = http_post_current(data);
	    else if(!strcmp(url, "/playlist"))
		page = http_post_playlist(data);
	    else if (!strncmp(url, "/playlist/", 10))
	    {
	        char *request = strdup(url);
	        page = http_post_playlist_item(request, data);
	        free(request);
	    }

            json_value_free(data);
            gui_update_playlist();
	}
    } else if(!strcmp(method, "DELETE"))
    {
	if(!strcmp(url, "/playlist"))
	    http_delete_playlist();
	else if(!strncmp(url, "/playlist/", 10))
	{
	    char *request = strdup(url);
	    page = http_delete_playlist_item(request);
	    free(request);
	}
        gui_update_playlist();
    }

    if(page == NULL)
            page = strdup("404 Not Found!");

    struct MHD_Response *response;
    response = MHD_create_response_from_data (strlen (page),
        (void*) page, true, false);

    int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return ret;
}

int http_controller_headers(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
    if(!strcmp(key, "Content-Length"))
    {
        int * length = (int*)cls;
        *length = atoi(value);
        return MHD_NO;
    }
    return MHD_YES;
}

char * http_get_playlist_item(char *url)
{
    char *uid = url + 10;

    songdata_song *song = http_song_by_uid(uid);
    if(song == NULL)
	    return NULL;

    return http_get_song_json(song);
}

char * http_get_current()
{
    if(!playlist->playing)
        return strdup("{}");

    char result[2560];
    char *file = http_get_song_json(playlist->playing);
    int duration = engine_get_length();
    int position = engine_get_elapsed();

    snprintf(result, 2560, "{\"file\":%s,\"duration\":%d,\"position\":%d}", file, duration, position);
    free(file);
    file = NULL;

    return strdup(result);
}

char * http_get_status()
{
    if(!playlist->playing)
        return strdup("{\"status\":\"stopped\"}");

    if(engine_is_playing())
        return strdup("{\"status\":\"playing\"}");

    return strdup("{\"status\":\"paused\"}");
}

char * http_get_playlist()
{
    char *builder = strdup("{\"files\":[");

    songdata_song *current = playlist->head;
    bool first = true;
    while(current)
    {
	char *file = http_get_song_json(current);

	char *newjson = malloc(strlen(file) + strlen(builder) + 2);
	strcpy(newjson, builder);
	if(!first)
		strcat(newjson, ",");

	strcat(newjson, file);
	free(file);
	file = NULL;
	free(builder);
	builder = newjson;

	current = current->next;
	first = false;
    }

    char *end = "]}";
    char *result = malloc(strlen(builder) + strlen(end) + 1);
    strcpy(result, builder);
    strcat(result, end);
    free(builder);
    builder = NULL;

    return result;
}

char * http_get_song_uid(songdata_song *song)
{
    char input[1024];
    snprintf(input, 1024, "%p", song);
    return strdup(input);
}

songdata_song * http_song_by_uid(char *uid)
{
    songdata_song *current = playlist->head;
    while(current != NULL)
    {
	if(!strcmp(http_get_song_uid(current), uid))
	    return current;
	current = current->next;
    }
    return NULL;
}

char * http_get_song_json(songdata_song *song)
{
    char *uid = http_get_song_uid(song);
    char file[1024];

    char *tag = song->tag;
    if(tag == NULL)
	    tag = strdup("");

    snprintf(file, 1024, "{\"uid\":\"%s\", \"location\":\"%s\", \"tag\":\"%s\"}", uid, song->fullpath, tag);
    free(uid);
    free(tag);
    uid = NULL;
    tag = NULL;
    return strdup(file);
}

char * http_json_extract(json_value *data, char * attribute)
{
    int i;
    json_value * value = NULL;

    for(i = 0; i < (*data).u.object.length; i++)
    {
        if(!strcmp((*data).u.object.values[i].name, attribute))
        {
            value = (*data).u.object.values[i].value;
        }
    }

    if(value == NULL)
        return NULL;

    int length = (*value).u.string.length;
    char * str = malloc(length + 1);
    strncpy(str, (*value).u.string.ptr, length);
    str[length] = 0;

    return str;
}

char * http_delete_playlist_item(char *url)
{
    char *uid = url + 10; //Skip /playlist/

    songdata_song *song = http_song_by_uid(uid);
    if(song == NULL)
	    return NULL;

    songdata_del(playlist, song);

    return strdup("");
}

char * http_post_playlist_item(char *url, json_value *data)
{
    char *location = http_json_extract(data, "location");
    char *tag = http_json_extract(data, "tag");
    if(location == NULL)
	    return NULL;
    if(tag == NULL)
	    tag = strdup("");

    char *uid = url + 10; //Skip /playlist/

    songdata_song *song = http_song_by_uid(uid);
    if(song == NULL)
	    return NULL;

    char *filename = strdup(location);
    char *path = split_filename(filename);
    
    songdata_song *newsong = new_songdata_song();
    newsong->fullpath = strdup(location);;
    newsong->filename = strdup(filename);
    newsong->path = strdup(path);
    newsong->title = strdup(filename);
    newsong->tag = tag;

    free(location);
    location = NULL;

    free(path);
    path = NULL;
    songdata_add(playlist, song, newsong);

    return NULL;
}

char * http_post_status(json_value *data)
{
    char * str = http_json_extract(data, "status");

    if(!strcmp(str, "next"))
        controller_next();

    if(!strcmp(str, "previous"))
        controller_prev();

    if(!strcmp(str, "stopped"))
        controller_stop();

    if(!strcmp(str, "playing"))
	if(!playlist->playing || !engine_is_playing())
	        controller_play_pause();

    if(!strcmp(str, "paused"))
	if(playlist->playing && engine_is_playing())
	        controller_play_pause();

    return strdup("");
}

char * http_post_current(json_value *data)
{
    char *uid = http_json_extract(data, "uid");

    songdata_song *entry = http_song_by_uid(uid);
    if(!entry)
	controller_jump_to_song(entry);
    free(uid);
}

char * http_post_playlist(json_value *data)
{
    char *fullpath = http_json_extract(data, "location");
    char *tag = http_json_extract(data, "tag");
    if(fullpath == NULL)
	return;
    if(tag == NULL)
	tag = strdup("");

    char *filename = strdup(fullpath);
    char *path = split_filename(filename);
    
    songdata_song *newsong = new_songdata_song();
    newsong->fullpath = fullpath;
    newsong->filename = strdup(filename);
    newsong->path = strdup(path);
    newsong->title = strdup(filename);
    newsong->tag = tag;

    free(path);

    songdata_add(playlist, playlist->tail, newsong);
}

void http_delete_playlist()
{
    controller_stop();
    songdata_clear ( playlist );
    gui_update_playlist();
    gui_update_info();
    update_panels ();
}

char* http_get_index()
{
    return strdup(
            "<!DOCTYPE html>\n"
            "<html lang=\"en\">\n"
            "<head>\n"
            "<title>MJS4</title>\n"
            "<link href=\"//netdna.bootstrapcdn.com/twitter-bootstrap/2.0.4/css/bootstrap-combined.min.css\" rel=\"stylesheet\">\n"
            "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js\"></script>\n"
            "</head>\n"
            "<body>\n"
            "<div class=\"container\">\n"
            "<h1>BolkPlayer</h1>\n"
            "<p>Welcome to MJS. This application can be controlled through an api:</p>\n"
            "<h3>GET /</h3>\n"
            "<p>Displays this message.</p>\n"
            "\n"
            "<h2>Playlist</h2>\n"
            "<p>The playlist contains all the media files that should be played.</p>\n"
            "\n"
            "<h3>GET /playlist</h3>\n"
            "<p>Returns a json representation of the current playlist.<br>\n"
            "Output:\n"
            "<pre>{\"files\": [{\"uid\": \"67dd3588-d684-11e1-b877-0001805c669b\", \"location\": \"/tmp/test.mp3\"}]}</pre>\n"
            "<strong>Test:</strong>\n"
            "<a href=\"#\" onclick=\"$.getJSON('/playlist', '', function(data){$('#getplaylist').html(JSON.stringify(data)); }); return false;\">get</a>\n"
            "<pre id=\"getplaylist\"></pre>\n"
            "</p>\n"
            "\n"
            "<h3>POST /playlist</h3>\n"
            "<p><pre>Content-type: application/json\n"
            "{ \"location\": \"/path/to/file.mp3\", \"tag\": \"tag1\" }</pre>\n"
            "Creates a new file at the end of the playlist.<br>\n"
            "<strong>Test:</strong><br><input type=\"text\" id=\"postplaylistinput\">, tag: <input type=\"text\" id=\"postplaylisttag\">\n"
            "<a href=\"#\" onclick=\"$.ajax({type: 'POST', url: '/playlist', data: JSON.stringify({'location':$('#postplaylistinput').val(), 'tag':$('#postplaylisttag').val()}), contentType: 'application/json'}); return false;\">post</a>\n"
            "</p>\n"
            "\n"
            "<h3>DELETE /playlist</h3>\n"
            "<p>Clears the playlists and stops the playback<br>\n"
            "<strong>Test:</strong><br>\n"
            "<a href=\"#\" onclick=\"$.ajax({url: '/playlist', type: 'DELETE' }); return false;\">delete</a>\n"
            "</p>\n"
            "\n"
            "<h2>File</h2>\n"
            "<p>Files can be individually viewed and removed based on the uid. It is also possible to insert files in the playlist at a specified position.\n"
            "</p>\n"
            "\n"
            "<h3>GET /playlist/<strong><em>uid</em></strong></h3>\n"
            "<p>Returns a json representation of the current file.<br>\n"
            "URL example: /playlist/67dd3588-d684-11e1-b877-0001805c669b<br>\n"
            "Output:\n"
            "<pre>{\"uid\": \"67dd3588-d684-11e1-b877-0001805c669b\", \"location\": \"/tmp/test.mp3\"}</pre>\n"
            "<strong>Test:</strong><br><input type=\"text\" id=\"getfileinput\">\n"
            "<a href=\"#\" onclick=\"$.getJSON('/playlist/' + $('#getfileinput').val(), '', function(data){$('#getfile').html(JSON.stringify(data)); }); return false;\">get</a>\n"
            "<pre id=\"getfile\"></pre>\n"
            "</p>\n"
            "\n"
            "<h3>DELETE /playlist/<strong><em>uid</em></strong></h3>\n"
            "<p>Deletes the file from the playlist.<br>\n"
            "URL example: /playlist/67dd3588-d684-11e1-b877-0001805c669b<br>\n"
            "<strong>Test:</strong><br><input type=\"text\" id=\"deletefileinput\">\n"
            "<a href=\"#\" onclick=\"$.ajax({url: '/playlist/' + $('#deletefileinput').val(), type: 'DELETE'}); return false;\">delete</a>\n"
            "</p>\n"
            "\n"
            "<h3>POST /playlist/<strong><em>uid</em></strong></h3>\n"
            "<p><pre>Content-type: application/json\n"
            "{ \"location\": \"/path/to/file.mp3\", \"tag\":\"tag2\" }</pre>\n"
            "Inserts a file before the selected file and redirects to the new resource.<br>\n"
            "URL example: /playlist/67dd3588-d684-11e1-b877-0001805c669b<br>\n"
            "<strong>Test:</strong><br><input type=\"text\" id=\"postfileinput1\">, path: <input type=\"text\" id=\"postfileinput2\">, tag: <input type=\"text\" id=\"postfileinput3\">\n"
            "<a href=\"#\" onclick=\"$.ajax({type: 'POST', url: '/playlist/' + $('#postfileinput1').val(), data: JSON.stringify({'location':$('#postfileinput2').val() , 'tag':$('#postfileinput3').val() }), contentType: 'application/json'}); return false;\">post</a>\n"
            "</p>\n"
            "\n"
            "<h2>Current file</h2>\n"
            "<p>This contains special info about the currently playing file.</p>\n"
            "\n"
            "<h3>GET /current</h3>\n"
            "<p>Returns data about the currently playing file, the duration of the file in seconds and the current position in the file (in seconds).<br>\n"
            "Output:\n"
            "<pre>{ file: {\"uid\": \"67dd3588-d684-11e1-b877-0001805c669b\", \"location\": \"/tmp/test.mp3\"},\n"
            " \"duration\": 136.231,\n"
            " \"position\": 45.364 }</pre>\n"
            "<strong>Test:</strong>\n"
            "<a href=\"#\" onclick=\"$.getJSON('/current', '', function(data){$('#getcurrent').html(JSON.stringify(data)); }); return false;\">get</a>\n"
            "<pre id=\"getcurrent\"></pre>\n"
            "</p>\n"
            "\n"
            "<h3>POST /current</h3>\n"
            "<p><pre>Content-type: application/json\n"
            "{ \"uid\": \"\"67dd3588-d684-11e1-b877-0001805c669b\" }</pre>\n"
            "Sets the specified uid as the current number and starts playing.\n"
            "<strong>Test:</strong><br><input type=\"text\" id=\"postcurrentinput\">\n"
            "<a href=\"#\" onclick=\"$.ajax({type: 'POST', url: '/current', data: JSON.stringify({'uid':$('#postcurrentinput').val() }), contentType: 'application/json'}); return false;\">post</a>\n"
            "\n"
            "<h2>Status</h2>\n"
            "<p>Data about the state of the player</p>\n"
            "\n"
            "<h3>GET /status</h3>\n"
            "<p>Returns data about the playback state.<br>\n"
            "Can be either:\n"
            "   <ul>\n"
            "   <li><strong>playing</strong>: Currently playing a file</li>\n"
            "   <li><strong>paused</strong>: Playback paused</li>\n"
            "   <li><strong>stopped</strong>: Playback stopped, no current file</li>\n"
            "   </ul>\n"
            "Output:\n"
            "<pre>{ \"state\": \"playing\" }</pre>\n"
            "<strong>Test:</strong>\n"
            "<a href=\"#\" onclick=\"$.getJSON('/status', '', function(data){$('#getstatus').html(JSON.stringify(data)); }); return false;\">get</a>\n"
            "<pre id=\"getstatus\"></pre>\n"
            "</p>\n"
            "\n"
            "<h3>POST /status</h3>\n"
            "<pre>Content-type: application/json\n"
            "{ \"status\": \"stopped\" }</pre>\n"
            "<p>Sets the playback state. Status field can be either of:\n"
            "   <ul>\n"
            "   <li><strong>paused</strong>: Playback paused</li>\n"
            "   <li><strong>stopped</strong>: Playback stopped, no current file</li>\n"
            "   <li><strong>playing</strong>: Currently playing a file</li>\n"
            "   <li><strong>next</strong>: Continues to the next file, state stays the same</li>\n"
            "   <li><strong>previous</strong>: Go to previous file, state stays the same</li>\n"
            "   </ul>\n"
            "<strong>Test:</strong><br>\n"
            "status: <input type=\"text\" id=\"poststatusinput\"><a href=\"#\" onclick=\"$.ajax({url: '/status', type: 'POST', data: JSON.stringify({'status': $('#poststatusinput').val()}), contentType: 'application/json' }); return false;\">post</a>\n"
            "</p>\n"
            "</div>\n"
            "</body>\n"
            "</html>\n"
    );
}
