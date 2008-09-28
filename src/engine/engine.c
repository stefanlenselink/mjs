#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <xine.h>
#include <curses.h>
#include <math.h>

#include "controller/controller.h"

xine_t * engine; // Main libxine object
xine_audio_port_t * ap; // The audio driver
xine_video_port_t * vp; // The video driver
xine_stream_t * stream; // Stream object

EngineState engine_state = engine_unitialized;
int length = 0;
int volume = 100;


Config * conf;


char * current_song;

/* Private internal functions */
static int is_safe(char);
static void url_encode(char *, char *);
static void xine_open_and_play(char *);
static void event_callback ( void *, const xine_event_t *);



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
    sprintf(tmp2, file);
  }
  xine_open ( stream, tmp2);
  xine_play ( stream, 0, 0 );
  if ( !xine_get_pos_length ( stream, &tmp, &tmp, &length ) )
  {
    length = 0;
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

void engine_ffwd ( int mill )
{
	if ( engine_state == engine_playing )
	{
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_FAST_4 );
		usleep ( mill * 1000 ); //TODO Shiften?
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL );
	}
}

void engine_frwd ( int mill )
{
	if ( engine_state == engine_playing )
	{
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_SLOW_4 );
		usleep ( mill * 1000 ); //TODO Shiften?
		xine_set_param ( stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL );
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

	// Now shut down cleanly
	xine_close_audio_driver ( engine, ap );
	xine_close_video_driver ( engine, vp );
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
