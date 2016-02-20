#include "defjams.h"

typedef void (*tune)();

typedef struct t_algoriddim 
{
  tune t;
} algo;

typedef struct melody_msg {
  int *melody;
  int osc_num;
  int melody_note_len;
  int melody_loop_len;
  int melody_play_lock;
  int melody_cur_note;
} melody_msg;


melody_msg* new_melody_msg(int *freqs, int melody_note_len, int loop_len);

sbmsg* new_sb_msg(void);
algo *new_algo(void);

void *algo_run(void *);
void *loop_run(void *);
void play_melody(const int osc_num, int *mlock, int *note, int *notes, const int note_len);
void mk_sbmsg_sine(int freq);
