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
#include "gui/gui.h"
#include "log.h"

xine_t * engine; // Main libxine object
xine_audio_port_t * ap; // The audio driver
xine_stream_t * stream; // Stream object

xine_stream_t * meta_stream; // Meta Stream object

xine_event_queue_t * queue; // The event queue running on the stream object.

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
	//TODO xine_get_file_extensions needs free
  return strcasestr("mp3 ogg flac acc wav voc",ext) && strcasestr(xine_get_file_extensions(engine), ext);
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
  char tmp3[1024] = "", tmp2[1024] = "";
  if(file == NULL){
    return;
  }
  if(file[0] == '/'){
    url_encode(file, tmp3);
    sprintf(tmp2, "file:/%s", tmp3);
  }else{
    sprintf(tmp2, "%s", file);
  }
  xine_close ( stream );
  if(!xine_open ( stream, tmp2)){
    char buf[512];
    sprintf(buf,"xine-open faild: %d", xine_get_error(stream) );
    //log_debug(buf);
    return;
  }
  if(!xine_play ( stream, 0, 0 )){
    char buf[512];
    sprintf(buf,"xine-play faild: %d", xine_get_error(stream) );
    //log_debug(buf);
    return;
  }
 int count = 0;
  while ( !xine_get_pos_length ( stream, &tmp, &tmp, &length ) ) // The header file states: "probably because it's not known yet... try again later"
  {
	sleepTS.tv_sec = 0;
	sleepTS.tv_nsec = 10000000;
	nanosleep(&sleepTS,NULL); //Just try until you get some usefull info
	count++;
	if(count>5) break;
    //log_debug("Sleeping");
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
      gui_progress_start((char *)prog->description);
    }else if(prog->percent == 100){
      gui_progress_stop();
    }else{
      int pcts = prog->percent;
      if(pcts < 0){
        pcts = 0;
      }else if(pcts > 100){
        pcts = 100;
      }
      gui_progress_animate();
      gui_progress_value(pcts);
    }
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

	// Create a new stream object
	stream = xine_stream_new ( engine, ap, NULL );

	meta_stream = xine_stream_new ( engine, NULL, NULL );

	//TODO in CFG zetten??
	xine_set_param ( stream, XINE_PARAM_EARLY_FINISHED_EVENT, 1 );

	queue = xine_event_new_queue ( stream );

	xine_event_create_listener_thread ( queue, event_callback, NULL );

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
    while( !xine_get_pos_length ( stream, &pos_stream , &pos_time , &length ) ) {
      nanosleep(&sleepTS,NULL);
      count++;
      if(count>5) return;
      if(engine_state != engine_playing) return;
    }
    if ( difftime(currentWind,previousWind) < 2) {
      windFactor *= (1 + (float) expFactor/1000);
    } else {
      windFactor = 1;
    }
    time(&previousWind);
    speedUp = speedUp * (int) windFactor;
    if(forward){
      xine_play( stream , 0 , pos_time + ( mill * speedUp) );
    }else{
      xine_play( stream , 0 , pos_time - ( mill * speedUp) );
    }
    nanosleep(&sleepTS,NULL);
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

static char * engine_load_meta_info_update_field(char * old, const char * new){
  if(old != NULL){
    free(old);
  }
  if(new == NULL)
    return NULL;
  return strdup(new);
}

static void engine_load_meta_info_from_stream(songdata_song * file, xine_stream_t * str){
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

void engine_load_current_meta_info(songdata_song * file){
  engine_load_meta_info_from_stream(file, stream);
}

void engine_load_meta_info(songdata_song * file){
  char * tmp3 = calloc(1024, sizeof(char));
  char * tmp2 = calloc(1024, sizeof(char));
  if(file->fullpath[0] == '/'){
    url_encode(file->fullpath, tmp3);
    sprintf(tmp2, "file:/%s", tmp3);
  }else{
    free(tmp3);
    free(tmp2);
    return; //We only handle files!
  }
  xine_open ( meta_stream, tmp2);
  engine_load_meta_info_from_stream(file, meta_stream);
  xine_close(meta_stream);
  free(tmp3);
  free(tmp2);
}
