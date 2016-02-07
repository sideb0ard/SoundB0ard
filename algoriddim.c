#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "algoriddim.h"
#include "bpmrrr.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;
extern bpmrrr *b;
extern GTABLE *sine_table;
extern GTABLE *tri_table;
extern GTABLE *square_table;
extern GTABLE *saw_up_table;
extern GTABLE *saw_down_table;

algo *new_algo()
{
  algo *a = NULL;
  a = calloc(1, sizeof(algo));
  if (a == NULL) {
    fprintf(stderr, "Nae memory for that song, mate..\n");
    return NULL;
  }
  return a;
}

sbmsg* new_sb_msg()
{
  sbmsg *msg = calloc(1, sizeof(sbmsg));
  return msg;
}

void mk_sbmsg_sine(int freq)
{
  sbmsg *m = new_sb_msg();
  strncpy(m->cmd, "timed_sig_start", 19);
  strncpy(m->params, "sine", 19);
  m->freq = freq;
  thrunner(m);
}


void play_melody(const int osc_num, int *mlock, int *note, int *notes, const int note_len)
{
  if (!*mlock) {

    *mlock = 1;

    if (rand() % 100 < 30) {
      printf("30%%!\n");
      //fade_runrrr(
      //vol_change(mixr, osc_num, 0.0);
      if (mixr->signals[osc_num]->vol >= 0.5) 
        faderrr(osc_num, DOWN);
    } else {
      if (mixr->signals[osc_num]->vol < 0.5) 
        faderrr(osc_num, UP);
      // vol_change(mixr, osc_num, 0.7);
      *note = (*note + 1) % note_len;
      freq_change(mixr, osc_num, (notes[*note])); 
    }
  }
}

melody_msg* new_melody_msg(int osc_num)
{
  melody_msg* m = calloc(1, sizeof(melody_msg));
  m->melody[0] = 440;
  m->melody[1] = 550;
  m->osc_num = 0;
  m->melody_len = 2;
  m->melody_play_lock = 0;
  m->melody_cur_note = 0;
  return m;
}

//void mk_mmsg(int freq)

void *algo_run(void *a)
{
  //(void) a;
  melody_msg *mmsg = (melody_msg*) a;

  printf("ALGO RUN CALLED - got me a msg: %d, %d, %d\n", mmsg->melody[0], mmsg->melody[1], mmsg->melody_len);

  mk_sbmsg_sine(mmsg->melody[0]);
  sleep(3);

  while (1)
  {
    if (b->cur_tick % 4 == 0) {
      play_melody(mmsg->osc_num, &mmsg->melody_play_lock, &mmsg->melody_cur_note, mmsg->melody, 2);
    } else if (mmsg->melody_play_lock) {
      printf("LOKIN\n");
      mmsg->melody_play_lock = 0;
    }
  }
}
