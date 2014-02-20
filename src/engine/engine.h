#ifndef _engine_h
#define _engine_h

#include "config/config.h"
#include "songdata/songdata.h"

typedef struct
{
	int eq_30;      /* equalizer gains -100..100   */
	int eq_60;      /* equalizer gains -100..100   */
	int eq_125;     /* equalizer gains -100..100   */
	int eq_250;     /* equalizer gains -100..100   */
	int eq_500;     /* equalizer gains -100..100   */
	int eq_1000;    /* equalizer gains -100..100   */
	int eq_2000;    /* equalizer gains -100..100   */
	int eq_4000;    /* equalizer gains -100..100   */
	int eq_8000;    /* equalizer gains -100..100   */
	int eq_16000;   /* equalizer gains -100..100   */
} Equalizer;

void engine_init ( Config * conf);
void engine_stop ( void );
void engine_jump_to ( char *);
int engine_is_paused(void);
int engine_is_playing(void);
void engine_resume_playback(void);
void engine_pause_playback(void);
void engine_next ( void );
void engine_prev ( void );
void engine_ffwd ( int , int );
void engine_frwd ( int , int );
void engine_shutdown ( void );
void engine_eq ( Equalizer * );
void engine_rva ( int );//Rva 100 = 100% = no change; range:  0<->200
void engine_volume ( int ); //vol = 0 .. 100
void engine_volume_up ( int );//vol = 0 .. 100
void engine_volume_down ( int );//vol = 0 .. 100
int engine_get_elapsed ( void );
int engine_get_remaining ( void );
int engine_get_length ( void );
void engine_load_meta_info(songdata_song *);
int engine_extention_is_supported(char *);

#endif /* _engine_h */
