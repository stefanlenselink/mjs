#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <xine.h>
#include <curses.h>
#include <math.h>

#include <unistd.h>

#include "controller/controller.h"
#include "gui/window_menubar.h"

xine_t * engine; // Main libxine object
xine_audio_port_t * ap; // The audio driver
xine_video_port_t * vp; // The video driver
xine_stream_t * stream; // Stream object

xine_stream_t * meta_stream; // Meta Stream object
xine_audio_port_t * meta_ap; // The audio driver
xine_video_port_t * meta_vp; // The video driver

EngineState engine_state = engine_unitialized;

int length = 0;
int volume = 100;

time_t previousWind;
float windFactor = 1.0;
struct timespec sleepTS;
Config * conf;


char * current_song;

/* Private internal functions */
static int is_safe(char);
static void url_encode(char *, char *);
static void xine_open_and_play(char *);
static void event_callback ( void *, const xine_event_t *);

int engine_extention_is_supported(char * ext)
{
  return strstr(xine_get_file_extensions(engine), ext) != NULL;
}

static int is_safe(char c)
{
  #define SAFE_LEN 18
  const char SAFE[SAFE_LEN] = ";?:@&=+$,-_.!~*'()";
  int i;
      if( ('a' <= c && c <= 'z')
      || ('A' <= c && c <= 'Z')
      || ('0' <= c && c <= '9') ){
    return 1;
      }else{
        for(i = 0; i < SAFE_LEN; i++)
        {
          if(c == SAFE[i]){
            return 1;
          }
        }
      }
      return 0;
}

static void url_encode(char * src, char * dest)
{
  int length = strlen(src);
  int i;
  const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
  for(i = 0; i < length; ++src)
  {
    i++;
    if (is_safe(*src))
      *dest++ = *src;
    else
    {
         // escape this char
      *dest++ = '%';
      *dest++ = DEC2HEX[*src >> 4];
      *dest++ = DEC2HEX[*src & 0x0F];
    }
  }
}
static void xine_open_and_play(char * file)
{
  int tmp;
  int status;
  char tmp3[1024] = "", tmp2[1024] = "";
  if(file == NULL){
    return;
  }
  if(file[0] == '/'){
    url_encode(file, tmp3);
    sprintf(tmp2, "file:/%s", tmp3);
  }else{
    sprintf(tmp2, file);
  }
  xine_close ( stream );
  xine_open ( stream, tmp2);
  xine_play ( stream, 0, 0 );
  while ( !xine_get_pos_length ( stream, &tmp, &tmp, &length ) ) // The header file states: "probably because it's not known yet... try again later"
  {
	sleepTS.tv_sec = 0;
	sleepTS.tv_nsec = 10000000;
	nanosleep(&sleepTS,NULL); //Just try until you get some usefull info
  }
}
static void event_callback ( void *user_data, const xine_event_t *event )
{
  if ( event->type == XINE_EVENT_UI_PLAYBACK_FINISHED &&  controller_has_next_song() ){
      current_song = controller_process_to_next_song();
      xine_set_param ( event->stream, XINE_PARAM_GAPLESS_SWITCH, 1 );
      xine_open_and_play(current_song);
  }else if(event->type == XINE_EVENT_UI_PLAYBACK_FINISHED &&  !controller_has_next_song()){
    engine_state = engine_stoped;
  }else if(event->type == XINE_EVENT_PROGRESS){
    xine_progress_data_t * prog = event->data;
    if(prog->percent == 0){
      window_menubar_progress_bar_init((char *)prog->description);
    }else if(prog->percent == 100){
      window_menubar_progress_bar_remove();
    }else{
      int pcts = prog->percent;
      if(pcts < 0){
        pcts = 0;
      }else if(pcts > 100){
        pcts = 100;
      }
      window_menubar_progress_bar_animate();
      window_menubar_progress_bar_progress(pcts);
    }
  }else if(event->type == XINE_EVENT_UI_SET_TITLE){
    window_play_notify_title_changed();
  }
}

void engine_init ( Config * init_conf)
{
	conf = init_conf;
	// Create our libxine engine, and initialise it
	engine = xine_new();
	xine_init ( engine );
    
	// Automatically choose an audio driver
	ap = xine_open_audio_driver ( engine, NULL, NULL );

	// We don't need a video driver
	vp = xine_open_video_driver ( engine, NULL, XINE_VISUAL_TYPE_NONE, NULL );

	// Create a new stream object
	stream = xine_stream_new ( engine, ap, vp );
    
    // Automatically choose an audio driver
    meta_ap = xine_open_audio_driver ( engine, NULL, NULL );

	// We don't need a video driver
    meta_vp = xine_open_video_driver ( engine, NULL, XINE_VISUAL_TYPE_NONE, NULL );
    meta_stream = xine_stream_new ( engine, meta_ap, meta_vp );

	//TODO in CFG zetten??
	xine_set_param ( stream, XINE_PARAM_EARLY_FINISHED_EVENT, 1 );


	xine_event_create_listener_thread ( xine_event_new_queue ( stream ), event_callback, NULL );

	volume = xine_get_param ( stream, XINE_PARAM_AUDIO_VOLUME );
    
    engine_state = engine_stoped;

}

void engine_stop ( void )
{
	// Stop the stream
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

int engine_is_paused(void){
  return engine_state == engine_paused;
}

int engine_is_playing(void){
  return engine_state == engine_playing;
}

void engine_resume_playback(void){
  if(engine_state == engine_paused && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_PAUSE){
    xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL );
    engine_state = engine_playing;
  }else if(engine_state == engine_paused && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_NORMAL){
    engine_state = engine_stoped; //TODO klopt deze aanname??
  }
}

void engine_pause_playback(void){
  if(engine_state == engine_playing && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_NORMAL){
    xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE );
    engine_state = engine_paused;
  }else if(engine_state == engine_playing && xine_get_param(stream, XINE_PARAM_SPEED) == XINE_SPEED_PAUSE){
    engine_state = engine_paused;
  }
}

void engine_ffwd ( int mill ,int expFactor )
{
	if ( engine_state == engine_playing )
	{
		/*
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_FAST_4 );
		usleep ( mill * 1000 ); //TODO Shiften?
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL );
		*/
		int pos_stream, pos_time, length_time;
		int status;
		int speedUp = 5;
		sleepTS.tv_sec = 0;
		sleepTS.tv_nsec = 10000000;
		time_t currentWind;
		time(&currentWind);
		while( !xine_get_pos_length ( stream, &pos_stream , &pos_time , &length_time ) ) {
			nanosleep(&sleepTS,NULL);
		}
		if ( difftime(currentWind,previousWind) < 2) {
			windFactor *= (1 + (float) expFactor/1000);
		} else {
			windFactor = 1;
		}
		time(&previousWind);
		speedUp = speedUp * (int) windFactor;
		xine_play( stream , 0 , pos_time + ( mill * speedUp) );
		//status = xine_get_status ( stream );
		nanosleep(&sleepTS,NULL);
	}
}

void engine_frwd ( int mill , int expFactor)
{
	if ( engine_state == engine_playing )
	{
		/*
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_SLOW_4 );
		usleep ( mill * 1000 ); //TODO Shiften?
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL );
		*/
		int pos_stream, pos_time, length_time;
		int status;
		int speedUp = 10;
		sleepTS.tv_sec = 0;
		sleepTS.tv_nsec = 10000000;
		time_t currentWind;
		time(&currentWind);
		while( !xine_get_pos_length ( stream, &pos_stream , &pos_time , &length_time ) ) {
			nanosleep(&sleepTS,NULL);
		}
		if ( difftime(currentWind,previousWind) < 2) {
			windFactor *= (1 + (float) expFactor/1000);
		} else {
			windFactor = 1;
		}
		time(&previousWind);
		speedUp = speedUp * (int) windFactor;
		xine_play( stream , 0 , pos_time - ( mill * speedUp) );
		//status = xine_get_status ( stream );
		nanosleep(&sleepTS,NULL);
	}
}

void engine_eq ( Equalizer * eq )
{
	xine_set_param ( stream, XINE_PARAM_EQ_30HZ, eq->eq_30 );
	xine_set_param ( stream, XINE_PARAM_EQ_60HZ, eq->eq_60 );
	xine_set_param ( stream, XINE_PARAM_EQ_125HZ, eq->eq_125 );
	xine_set_param ( stream, XINE_PARAM_EQ_250HZ, eq->eq_250 );
	xine_set_param ( stream, XINE_PARAM_EQ_500HZ, eq->eq_500 );
	xine_set_param ( stream, XINE_PARAM_EQ_1000HZ, eq->eq_1000 );
	xine_set_param ( stream, XINE_PARAM_EQ_2000HZ, eq->eq_2000 );
	xine_set_param ( stream, XINE_PARAM_EQ_4000HZ, eq->eq_4000 );
	xine_set_param ( stream, XINE_PARAM_EQ_8000HZ, eq->eq_8000 );
	xine_set_param ( stream, XINE_PARAM_EQ_16000HZ, eq->eq_16000 );
}

//Rva 100 = 100% = no change 0<->200
void engine_rva ( int rva )
{
	if ( rva >= 0 && rva <= 200 )
	{
		xine_set_param ( stream, XINE_PARAM_AUDIO_AMP_LEVEL, rva );
	}
}

//vol = 0 .. 100
void engine_volume ( int vol )
{
	if ( vol >= 0 && vol <= 100 )
	{
		xine_set_param ( stream, XINE_PARAM_AUDIO_VOLUME, vol );
		volume = vol;
	}
}

void engine_volume_up ( int ammount )
{
	if ( ammount >= 0 && ammount <= 100 )
	{
		volume += ammount;
		if ( volume > 100 )
		{
			volume = 100;
		}
		engine_volume ( volume );
	}
}

void engine_volume_down ( int ammount )
{
	if ( ammount >= 0 && ammount <= 100 )
	{
		volume -= ammount;
		if ( volume < 0 )
		{
			volume = 0;
		}
		engine_volume ( volume );
	}
}

void engine_shutdown ( void )
{
	// Stop playing and close down the stream
	xine_close ( stream );
    
    xine_close( meta_stream );

	// Now shut down cleanly
	xine_close_audio_driver ( engine, ap );
	xine_close_video_driver ( engine, vp );
    
    	// Now shut down cleanly
    xine_close_audio_driver ( engine, meta_ap );
    xine_close_video_driver ( engine, meta_vp );
    
	xine_exit ( engine );
}

int engine_get_elapsed ( void )
{
	int a, b, c;
	if ( !xine_get_pos_length ( stream, &a, &b, &c ) )
	{
		return 0;
	}
	return b / 1000;
}
int engine_get_remaining ( void )
{
	int a, b, c;
	if ( !xine_get_pos_length ( stream, &a, &b, &c ) )
	{
		return 0;
	}
	return ( c - b ) / 1000;
}

int engine_get_length ( void )
{
	return length / 1000;
}

static char * engine_load_meta_info_update_field(char * old, const char * new){
  if(old != NULL){
    free(old);
  }
  if(new == NULL)
    return NULL;
  return strdup(new);
}

static void engine_load_meta_info_from_stream(flist * file, xine_stream_t * str){
  const char * tmp;
  tmp = xine_get_meta_info(str, XINE_META_INFO_TITLE);
  file->title = engine_load_meta_info_update_field(file->title, tmp);
  tmp = xine_get_meta_info(str, XINE_META_INFO_ARTIST);
  file->artist = engine_load_meta_info_update_field(file->artist, tmp);
  tmp = xine_get_meta_info(str, XINE_META_INFO_ALBUM);
  file->album = engine_load_meta_info_update_field(file->album, tmp);
  tmp = xine_get_meta_info(str, XINE_META_INFO_GENRE);
  file->genre = engine_load_meta_info_update_field(file->genre, tmp);
  
}

void engine_load_current_meta_info(flist * file){
  engine_load_meta_info_from_stream(file, stream);
}

void engine_load_meta_info(flist * file){
  char tmp3[1024] = "", tmp2[1024] = "";
  if(file->fullpath[0] == '/'){
    url_encode(file->fullpath, tmp3);
    sprintf(tmp2, "file:/%s", tmp3);
  }else{
    return; //We only handle files!
  }
  
  xine_open ( meta_stream, tmp2);
  engine_load_meta_info_from_stream(file, meta_stream);
}

