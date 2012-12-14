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
#include <microhttpd.h>

struct MHD_Daemon *http_daemon;

void http_controller_init(Config * cnf) {
    http_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 8080, NULL, NULL, &http_controller_request, NULL, MHD_OPTION_END);
}

void http_controller_shutdown(void) {
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
        {
            page = http_get_index();
        }
	else if (!strcmp(url, "/status"))
	{
	    page = http_get_status();
	}
    } else if(!strcmp(method, "POST"))
    {
        //BUG: POSTs seem to be aborted, no data is sent back. Functions get executed though not very tidy

        //Calculate content_length
        int content_length = 0;
        MHD_get_connection_values(connection, MHD_HEADER_KIND, http_controller_headers, &content_length);
        
        //We need more post data
        if(content_length > *upload_data_size)
            return MHD_YES;

        //Parse JSON
        json_value *data = json_parse(upload_data);

        fprintf(stderr, "POST %s (%s)\n\r", url, upload_data);

        //Execute commands
        if(!strcmp(url, "/status"))
        {
            http_post_status(data);
        }

        json_value_free(data);
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

char * http_get_status()
{
    if(!playlist->playing)
        return strdup("{\"status\":\"stopped\"}");

    if(engine_is_playing())
        return strdup("{\"status\":\"playing\"}");

    return strdup("{\"status\":\"paused\"}");
}

void http_post_status(json_value *data)
{
    int i;
    json_value * nextstatus = NULL;
    
    for(i = 0; i < (*data).u.object.length; i++)
    {
        if(!strcmp((*data).u.object.values[i].name, "status"))
        {
            nextstatus = (*data).u.object.values[i].value;
        }
    }

    if(nextstatus == NULL)
        return;
    char * str = malloc((*nextstatus).u.string.length + 1);
    strncpy(str, (*nextstatus).u.string.ptr, (*nextstatus).u.string.length);
    str[(*nextstatus).u.string.length] = 0;

    fprintf(stderr, "action: %s\n\r", str);

    if(!strcmp(str, "next"))
        controller_next();

    if(!strcmp(str, "previous"))
        controller_prev();

    if(!strcmp(str, "stopped"))
        controller_stop();

    if(!strcmp(str, "playing"))
        controller_play_pause();

    if(!strcmp(str, "paused"))
        controller_play_pause();
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
            "<a href=\"#\" onclick=\"$.getJSON('/playlist', '', function(data){$('#getplaylist').html(JSON.stringify(data)); }}); return false;\">get</a>\n"
            "<pre id=\"getplaylist\"></pre>\n"
            "</p>\n"
            "\n"
            "<h3>POST /playlist</h3>\n"
            "<p><pre>Content-type: application/json\n"
            "{ \"location\": \"/path/to/file.mp3\" }</pre>\n"
            "Creates a new file at the end of the playlist and redirects to the created file.<br>\n"
            "<strong>Test:</strong><br><input type=\"text\" id=\"postplaylistinput\">\n"
            "<a href=\"#\" onclick=\"$.ajax({type: 'POST', url: '/playlist', data: JSON.stringify({'location':$('#postplaylistinput').val() }), contentType: 'application/json'}); return false;\">post</a>\n"
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
            "{ \"location\": \"/path/to/file.mp3\" }</pre>\n"
            "Inserts a file before the selected file and redirects to the new resource.<br>\n"
            "URL example: /playlist/67dd3588-d684-11e1-b877-0001805c669b<br>\n"
            "<strong>Test:</strong><br><input type=\"text\" id=\"postfileinput1\">, path: <input type=\"text\" id=\"postfileinput2\">\n"
            "<a href=\"#\" onclick=\"$.ajax({type: 'POST', url: '/playlist/' + $('#postfileinput1').val(), data: JSON.stringify({'location':$('#postfileinput2').val() }), contentType: 'application/json'}); return false;\">post</a>\n"
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
            "<a href=\"#\" onclick=\"$.ajax({url: '/current', success: function(data){$('#getcurrent').html(JSON.stringify(data)); }}); return false;\">get</a>\n"
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
