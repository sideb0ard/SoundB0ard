#include "defjams.h"

typedef void (*tune)();

typedef struct t_algoriddim 
{
  tune t;
} algo;

typedef struct melody_msg {
  double *melody;
  int osc_num;
  int melody_note_len;
  int melody_loop_len;
  int melody_play_lock;
  int melody_cur_note;
  double mod_freq; // modulator for an FM message
} melody_msg;


melody_msg* new_melody_msg(double *freqs, int melody_note_len, int loop_len);

algo *new_algo(void);

void *algo_run(void *);
void *loop_run(void *);
void *floop_run(void *);
void *randdrum_run(void *m);
void play_melody(const int osc_num, int *mlock, int *note, double *notes, const int note_len);
void fplay_melody(const int sg_num, int *mlock, int *note, double *notes, const int note_len);
void mk_sbmsg_sine(int freq);
