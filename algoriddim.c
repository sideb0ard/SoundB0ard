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

//sbmsg* new_sb_msg()
//{
//  sbmsg *msg = calloc(1, sizeof(sbmsg));
//  return msg;
//}
//
//void mk_sbmsg_sine(int freq)
//{
//  sbmsg *m = new_sb_msg();
//  strncpy(m->cmd, "timed_sig_start", 19);
//  strncpy(m->params, "sine", 19);
//  m->freq = freq;
//  thrunner(m);
//}


void play_melody(const int osc_num, int *mlock, int *note, int *notes, const int note_len)
//void *play_melody(void *mm)
{
  if (!*mlock) {

    *mlock = 1;
    //  //fade_runrrr(
    //  //vol_change(mixr, osc_num, 0.0);
    //  if (mixr->signals[osc_num]->vol >= 0.5) 
    //    faderrr(osc_num, DOWN);
    //} else {
      //if (mixr->signals[osc_num]->vol < 0.5) 
      //  faderrr(osc_num, UP);
      // vol_change(mixr, osc_num, 0.7);
      *note = (*note + 1) % note_len;
      freq_change(mixr, osc_num, (notes[*note])); 
    //}
    /// play_melody(mmsg->osc_num, &mmsg->melody_play_lock, &mmsg->melody_cur_note, mmsg->melody, mmsg->melody_note_len);
  }
}

melody_msg* new_melody_msg(int* freqs, int melody_note_len, int loop_len)
{
  melody_msg* m = calloc(1, sizeof(melody_msg));
  m->melody = freqs;
  m->osc_num = 0;
  m->melody_note_len = melody_note_len;
  m->melody_loop_len = loop_len;
  m->melody_play_lock = 0;
  m->melody_cur_note = 0;
  return m;
}

//void mk_mmsg(int freq)

void *algo_run(void *a)
{
  //(void) a;
  melody_msg *mmsg = (melody_msg*) a;

  printf("ALGO RUN CALLED - got me a msg: %d, %d, %d\n", mmsg->melody[0], mmsg->melody[1], mmsg->melody_note_len);

  //mk_sbmsg_sine(mmsg->melody[0]);
  int osc_num = add_osc(mixr, mmsg->melody[0], sine_table);
  mmsg->osc_num = osc_num;
  do {} while (b->cur_tick % 16 != 0);
  faderrr(osc_num, UP);
  sleep(3);

  while (1)
  {
    if (b->cur_tick % (4*(mmsg->melody_loop_len)) == 0) {
      play_melody(mmsg->osc_num, &mmsg->melody_play_lock, &mmsg->melody_cur_note, mmsg->melody, mmsg->melody_note_len);
    } else if (mmsg->melody_play_lock) {
      mmsg->melody_play_lock = 0;
    }
  }

  //(void) a;
  //int osc_one_lock = 0;
  //int osc_two_lock = 0;
  //int osc_one = add_osc(mixr, 427, sine_table);
  //int osc_two = add_osc(mixr, 427, sine_table);

  //do {} while (b->cur_tick % 16 != 0);

  //faderrr(osc_one, UP);
  //faderrr(osc_two, UP);
  //sleep(3);

  //while (1)
  //{
  //  if ((b->cur_tick % (4*4) == 0) && !osc_one_lock) {
  //    freq_change(mixr, osc_one, rand() % 100 + 400);
  //    osc_one_lock = 1;
  //  } else if (osc_one_lock == 1) {
  //    osc_one_lock = 0;
  //  }
  //  if ((b->cur_tick % (4*8) == 0) && !osc_two_lock) {
  //    freq_change(mixr, osc_two, rand() % 100 + 400);
  //    osc_two_lock = 1;
  //  } else if (osc_two_lock == 1) {
  //    osc_two_lock = 0;
  //  }
  //}
}
