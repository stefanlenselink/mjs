#ifndef _engine_h
#define _engine_h

typedef struct {
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
    
void engine_init(void);
void engine_stop(void);
void engine_play(void);
void engine_next(void);
void engine_prev(void);
void engine_ffwd(int);
void engine_frwd(int);
void engine_shutdown(void);
void engine_eq(Equalizer *);
void engine_rva(int);//Rva 100 = 100% = no change; range:  0<->200
void engine_volume(int); //vol = 0 .. 100
void engine_volume_up(int);//vol = 0 .. 100
void engine_volume_down(int);//vol = 0 .. 100

#endif /* _engine_h */
