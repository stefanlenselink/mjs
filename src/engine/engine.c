#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <xine.h>
#include <time.h>
#include <unistd.h>

#include <apreq_util.h> //TODO: Get rid of apache dependency.

#include "controller/controller.h" //TODO: get rid of this.
#include "gui/gui.h"			   //TODO: get rid of this.
#include "log.h"
#include "utils.h"

//MJS config.
Config * conf;

//libxine variables.
static xine_t * engine; // Main libxine object
static xine_audio_port_t * ap; // The audio driver
static xine_stream_t * stream; // Stream object
static xine_stream_t * meta_stream; // Meta Stream object
static xine_event_queue_t * queue; // The event queue running on the stream object.


//Possible engine states:
typedef enum{
	engine_paused,
	engine_playing,
	engine_stoped,
	engine_unitialized
} EngineState;

//Current state of the engine:
static EngineState engine_state = engine_unitialized;

//Song data:
static int length = 0;
static char * current_song;
static time_t previousWind;
static float windFactor = 1.0;

static struct timespec sleepTS;


/* Private internal functions */
//Playback private helpers:
static void engine_fwd( int, int, char );
static void xine_open_and_play( char * );
static void url_encode( char *, char * );
static void event_callback( void *, const xine_event_t * );

//Metadata private helpers:
static void engine_load_meta_info_from_stream( songdata_song *, xine_stream_t * );
static char * engine_load_meta_info_update_field( char *, xine_stream_t *, int );


//*********************************
// Utility functions
//
void engine_init ( Config * init_conf)
{
	conf = init_conf;

	// Create our libxine engine, and initialise it
	engine = xine_new();
	xine_init ( engine );

	// Automatically choose an audio driver
	ap = xine_open_audio_driver ( engine, NULL, NULL );

	// Create a new stream object
	stream = xine_stream_new ( engine, ap, NULL );
	meta_stream = xine_stream_new ( engine, NULL, NULL );

	//TODO in CFG zetten??
	xine_set_param ( stream, XINE_PARAM_EARLY_FINISHED_EVENT, 1 );

	queue = xine_event_new_queue ( stream );
	xine_event_create_listener_thread ( queue, event_callback, NULL );

	engine_state = engine_stoped;
}

void engine_shutdown ( void )
{
	// Stop playing and close down the stream
	xine_stop( stream );
	xine_stop( meta_stream );

	xine_event_dispose_queue(queue);

	xine_close(stream);
	xine_close(meta_stream);

	xine_dispose(stream);
	xine_dispose(meta_stream);

	// Now shut down cleanly
	xine_close_audio_driver ( engine, ap );

    // Now shut down cleanly
	xine_exit ( engine );
}

int engine_extention_is_supported( char * ext )
{
	char * xine_file_extensions = xine_get_file_extensions(engine);
	int res = strcasestr("mp3 ogg flac acc wav voc", ext) && strcasestr(xine_file_extensions, ext);
	free(xine_file_extensions);

	return res;
}


//************************************
//Engine playback controls.
//
void engine_stop ( void )
{
	xine_stop ( stream );
    xine_close ( stream );
    length = 0;
    engine_state = engine_stoped;
}

void engine_jump_to ( char * current)
{
	xine_stop ( stream );
	xine_close ( stream );
	xine_open_and_play(current);
	current_song = current;
	engine_state = engine_playing;
}

void engine_resume_playback(void)
{
	if(engine_state == engine_paused && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_PAUSE)
	{
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL );
		engine_state = engine_playing;
	}
	else if(engine_state == engine_paused && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_NORMAL)
	{
		engine_state = engine_stoped; //TODO klopt deze aanname??
	}
}

void engine_pause_playback(void)
{
	if(engine_state == engine_playing && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_NORMAL)
	{
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE );
		engine_state = engine_paused;
	}
	else if(engine_state == engine_playing && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_PAUSE)
	{
		engine_state = engine_paused;
	}
}

void engine_ffwd ( int mill ,int expFactor )
{
	engine_fwd(mill, expFactor, 1);
}

void engine_frwd ( int mill , int expFactor)
{
	engine_fwd(mill, expFactor, 0);
}

static void engine_fwd(int mill, int expFactor, char forward)
{
	if ( engine_state == engine_playing )
	{
		int pos_stream, pos_time;
		int speedUp = 5;
		sleepTS.tv_sec = 0;
		sleepTS.tv_nsec = 20000000;
		time_t currentWind;
		time(&currentWind);
		int count = 0;

		while( !xine_get_pos_length ( stream, &pos_stream , &pos_time , &length ) )
		{
			nanosleep(&sleepTS,NULL);
			count++;
			if(count>5)
				return;
			if(engine_state != engine_playing)
				return;
		}

		if ( difftime(currentWind,previousWind) < 2)
		{
			windFactor *= (1 + (float) expFactor/1000);
		}
		else
		{
			windFactor = 1;
		}

		time(&previousWind);
		speedUp = speedUp * (int) windFactor;
		if(forward)
		{
			xine_play( stream , 0 , pos_time + ( mill * speedUp) );
		}
		else
		{
			xine_play( stream , 0 , pos_time - ( mill * speedUp) );
		}
		nanosleep(&sleepTS,NULL);
	}
}

static void url_encode(char * src, char * dest)
{
	char * apreq_encoded_dest = malloc(sizeof(char) * ((3* strlen(src)) + 1));
	char * start_loc = apreq_encoded_dest;
	apreq_encode(apreq_encoded_dest, src, strlen(src));
	int i;
	int length = strlen(apreq_encoded_dest);
	for(i = 0; i < length; ++apreq_encoded_dest)
	{
		i++;
		if (*apreq_encoded_dest != '+'){
			*dest++ = *apreq_encoded_dest;
		}else{
			// escape this char
			*dest++ = '%';
			*dest++ = '2';
			*dest++ = '0';
		}
	}
	//Add termination char as last always.
	*dest++ = '\0';
	free(start_loc);
}

static void xine_open_and_play(char * file)
{
	if(file == NULL)
	{
		return;
	}

	char * file_uri = malloc(((sizeof(char) * strlen(file) * 3)) + 7);

	if(file[0] == '/')
	{
 		char * file_encoded =  malloc(((sizeof(char) * strlen(file) * 3)) + 1);
		url_encode(file, file_encoded);
		sprintf(file_uri, "file:/%s", file_encoded);
		free(file_encoded);
	}
	else
	{
		sprintf(file_uri, "%s", file);
	}
	xine_close ( stream );
	log_debug(file_uri);

	if(!xine_open ( stream, file_uri)){
		return;
	}
	free(file_uri);

	if(!xine_play ( stream, 0, 0 )){
		return;
	}

	int count = 0;
	int dummy1, dummy2;
	// The header file states: "probably because it's not known yet... try again later"
	while ( !xine_get_pos_length ( stream, &dummy1, &dummy2, &length ) )
	{
		sleepTS.tv_sec = 0;
		sleepTS.tv_nsec = 10000000;
		nanosleep(&sleepTS,NULL); //Just try until you get some usefull info
		count++;
		if(count>5) break;
		//log_debug("Sleeping");
	}
}

static void event_callback(void *user_data, const xine_event_t *event) {
	xine_progress_data_t *prog;

	switch (event->type) {
	case XINE_EVENT_UI_PLAYBACK_FINISHED:
		current_song = controller_process_to_next_song();
		if (current_song == NULL )
		{
			engine_state = engine_stoped;
		}
		else
		{
			xine_set_param(event->stream, XINE_PARAM_GAPLESS_SWITCH, 1);
			xine_open_and_play(current_song);
		}
		break;
	case XINE_EVENT_PROGRESS:
		prog = event->data;
		if (prog->percent == 0)
		{
			gui_progress_start((char *) prog->description);
		}
		else if (prog->percent == 100)
		{
			gui_progress_stop();
		}
		else
		{
			int pcts = prog->percent;
			if (pcts < 0)
			{
				pcts = 0;
			}
			else if (pcts > 100)
			{
				pcts = 100;
			}
			gui_progress_animate();
			gui_progress_value(pcts);
		}
		break;
	}
}


//*****************************
// Engine state information.
//
int engine_is_paused(void)
{
	return engine_state == engine_paused;
}

int engine_is_playing(void)
{
	return engine_state == engine_playing;
}

int engine_get_elapsed ( void )
{
	int a, b;
    if ( !xine_get_pos_length ( stream, &a, &b, &length ) )
	{
		return 0;
	}
	return b / 1000;
}

int engine_get_remaining ( void )
{
	int a, b;
    if ( !xine_get_pos_length ( stream, &a, &b, &length ) && !length)
	{
		return 0;
	}
	return ( length - b ) / 1000;
}

int engine_get_length ( void )
{
	return length / 1000;
}


//******************************
// Metadata
//
void engine_load_meta_info(songdata_song * file)
{
	if(file->fullpath[0] != '/')
		return;


	char * tmp3 = calloc((strlen(file->fullpath) * 3) + 1, sizeof(char));
	url_encode(file->fullpath, tmp3);
	char * tmp2 = calloc(strlen(tmp3) + 7, sizeof(char));
	sprintf(tmp2, "file:/%s", tmp3);
	free(tmp3);

	if(!xine_open ( meta_stream, tmp2))
	{
		free(tmp2);
		return;
	}

	engine_load_meta_info_from_stream(file, meta_stream);
	xine_close(meta_stream);
	free(tmp2);
}

static char * engine_load_meta_info_update_field(char * old, xine_stream_t *stream, int info)
{
	const char * tmp = xine_get_meta_info(stream, info);
	if(tmp == NULL || !strlen(tmp))
		return NULL;

	//Allowed because strtrim returns old pointer ;).
	char * clean = strtrim(strdup(tmp));

	if(old != NULL)
		free(old);

	char * result = strdup(clean);
	free(clean);

	return result;
}

static void engine_load_meta_info_from_stream(songdata_song * file, xine_stream_t * stream)
{
	char * tmp = engine_load_meta_info_update_field(file->title, stream, XINE_META_INFO_TITLE);
	if(tmp)
		file->title = tmp;

	tmp = engine_load_meta_info_update_field(file->artist, stream, XINE_META_INFO_ARTIST);
	if(tmp)
		file->artist = tmp;

	tmp = engine_load_meta_info_update_field(file->album, stream, XINE_META_INFO_ALBUM);
	if(tmp)
		file->album = tmp;

	tmp = engine_load_meta_info_update_field(file->genre, stream, XINE_META_INFO_GENRE);
	if(tmp)
		file->genre = tmp;

}

