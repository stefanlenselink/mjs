#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <xine.h>
#include <curses.h>

xine_t * engine; // Main libxine object
xine_audio_port_t * ap; // The audio driver
xine_video_port_t * vp; // The video driver
xine_stream_t * stream; // Stream object

 int playing = 0;
 int paused = 0;
 
 int volume = 100;

Config * conf;
 
 
wlist * playlist;

static void test_cb(void *user_data, const xine_event_t *event)
{
  
  printf("\nHallo:  %d!!!\n", event->type);
  if(event->type == XINE_EVENT_UI_PLAYBACK_FINISHED &&  playlist->playing->next != NULL && playlist->playing->next != playlist->playing){
    printf("\n Playback finished!!!!\n");
    xine_set_param(event->stream, XINE_PARAM_GAPLESS_SWITCH, 1);
    xine_open(event->stream, playlist->playing->next->fullpath);
    xine_play(stream, 0, 0);
    // TODO Update the controller!
  }
}

void engine_init(Config * init_conf, wlist * init_playlist)
{
  conf = init_conf;
  playlist = init_playlist;
  // Create our libxine engine, and initialise it
  engine = xine_new();
  xine_init(engine);

  // Automatically choose an audio driver
  ap = xine_open_audio_driver(engine, NULL, NULL);

  // We don't need a video driver
  vp = xine_open_video_driver(engine, NULL, XINE_VISUAL_TYPE_NONE, NULL);

  // Create a new stream object
  stream = xine_stream_new(engine, ap, vp);
  
  //TODO in CFG zetten??
  xine_set_param(stream, XINE_PARAM_EARLY_FINISHED_EVENT, 1);
  
  
  xine_event_create_listener_thread(xine_event_new_queue(stream), test_cb, NULL);
  
  playing = 0;
  paused = 0;
  volume = xine_get_param(stream, XINE_PARAM_AUDIO_VOLUME);
}

void engine_stop(void)
{
  // Stop the stream
  xine_stop(stream);
  playing = 0;
  paused = 0;
}

void engine_play(void)
{ 
  if (playing){
    xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
    playing = 0;
    paused = 1;
  }else if(paused){
    playing = 1;
    xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
    paused = 0;
  }else{
    playing = 1;
    paused = 0;
    
    xine_stop(stream);
    
    xine_close(stream);
    
    // Open the stream we want to listen to
    xine_open(stream, playlist->playing->fullpath);
    
    // Play the stream
    xine_play(stream, 0, 0);
  }
}

void engine_next(void)
{
  playing = 0;
  paused = 0;
  //TODO shift nr
  engine_play();
}

void engine_prev(void)
{
  playing = 0;
  paused = 0;
  //TODO shift nr
  engine_play();
}

void engine_ffwd(int mill)
{
  if(playing){
    xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_FAST_4);
    usleep(mill * 1000); //TODO Shiften?
    xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
  }
}

void engine_frwd(int mill)
{
  if(playing){
    xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_SLOW_4);
    usleep(mill * 1000); //TODO Shiften?
    xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
  }
}

void engine_eq(Equalizer * eq)
{
  xine_set_param(stream, XINE_PARAM_EQ_30HZ, eq->eq_30);
  xine_set_param(stream, XINE_PARAM_EQ_60HZ, eq->eq_60);
  xine_set_param(stream, XINE_PARAM_EQ_125HZ, eq->eq_125);
  xine_set_param(stream, XINE_PARAM_EQ_250HZ, eq->eq_250);
  xine_set_param(stream, XINE_PARAM_EQ_500HZ, eq->eq_500);
  xine_set_param(stream, XINE_PARAM_EQ_1000HZ, eq->eq_1000);
  xine_set_param(stream, XINE_PARAM_EQ_2000HZ, eq->eq_2000);
  xine_set_param(stream, XINE_PARAM_EQ_4000HZ, eq->eq_4000);
  xine_set_param(stream, XINE_PARAM_EQ_8000HZ, eq->eq_8000);
  xine_set_param(stream, XINE_PARAM_EQ_16000HZ, eq->eq_16000);
}

//Rva 100 = 100% = no change 0<->200
void engine_rva(int rva)
{
  if(rva >= 0 && rva <= 200){
    xine_set_param(stream, XINE_PARAM_AUDIO_AMP_LEVEL, rva);
  }
}

//vol = 0 .. 100
void engine_volume(int vol)
{
  if(vol >= 0 && vol <= 100){
    xine_set_param(stream, XINE_PARAM_AUDIO_VOLUME, vol);
    volume = vol;
  }
}

void engine_volume_up(int ammount)
{
  if(ammount >= 0 && ammount <= 100){
    volume += ammount;
    if(volume > 100){
      volume = 100;
    }
    engine_volume(volume);
  }
}

void engine_volume_down(int ammount)
{
  if(ammount >= 0 && ammount <= 100){
    volume -= ammount;
    if(volume < 0){
      volume = 0;
    }
    engine_volume(volume);
  }
}

void engine_shutdown(void)
{
  // Stop playing and close down the stream
  xine_close(stream);

  // Now shut down cleanly
  xine_close_audio_driver(engine, ap);
  xine_close_video_driver(engine, vp);
  xine_exit(engine);
}

int engine_get_elapsed(void)
{
  int a, b, c;
  if(!xine_get_pos_length(stream, &a, &b, &c)){
    return 0;
  }
  return (int) b / 1000;
}
int engine_get_remaining(void)
{
  int a, b, c;
  if(!xine_get_pos_length(stream, &a, &b, &c)){
    return 0;
  }
  return (int)(c - b) / 1000;
}
