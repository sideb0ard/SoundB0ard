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

void mk_sbmsg_sine(int freq)
{
  sbmsg *m = new_sb_msg();
  strncpy(m->cmd, "timed_sig_start", 19);
  strncpy(m->params, "sine", 19);
  m->freq = freq;
  thrunner(m);
}

void *algo_run(void *a)
{
  (void) a;

  int melody[] = {299, 270, 399, 356};
  int melody_play_lock = 0;
  int melody_cur_note = 0;

  int bass_melody[] = {180, 150};
  int bass_play_lock = 0;
  int bass_cur_note = 0;

  mk_sbmsg_sine(227);
  sleep(3);

  mk_sbmsg_sine(230);
  sleep(3);

  mk_sbmsg_sine(180);
  sleep(3);

  mk_sbmsg_sine(299);
  sleep(3);

  while (1)
  {
    if (b->cur_tick % 32 == 0) {
      play_melody(2, &bass_play_lock, &bass_cur_note, bass_melody, 2);
    } else if (bass_play_lock) {
      bass_play_lock = 0;
    }
    if (b->cur_tick % 16 == 0) {
      play_melody(3, &melody_play_lock, &melody_cur_note, melody, 4);
    } else if (melody_play_lock) {
      melody_play_lock = 0;
    }
  }
}
