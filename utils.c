#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "mixer.h"
#include "utils.h"

extern bpmrrr *b;
extern mixer *mixr;
extern GTABLE *sine_table;

void *timed_sig_start(void * arg)
{
  sig_msg *sig = arg;
  do {} while (b->cur_tick % 16 != 0);
  int o = add_osc(mixr, sig->freq, sine_table);
  startrrr(o);
  free(sig);
  return NULL;
}

void startrrr(int sig_num)
{
  double vol = mixr->signals[sig_num]->vol;
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 5000000;
  while (vol < 1.0) {
    vol += 0.001;
    volfunc(mixr->signals[sig_num], vol);
    nanosleep(&ts, NULL);
  }
}
