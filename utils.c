#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "mixer.h"
#include "utils.h"

extern bpmrrr *b;
extern mixer *mixr;
extern GTABLE *sine_table;
extern GTABLE *saw_down_table;
extern GTABLE *saw_up_table;
extern GTABLE *tri_table;
extern GTABLE *square_table;

void *timed_sig_start(void * arg)
{
  sbmsg *msg = arg;
  int osc_num = 0;

  do {} while (b->cur_tick % 16 != 0);

  if (strcmp(msg->params, "sine") == 0) {
      osc_num = add_osc(mixr, msg->freq, sine_table);
  } else if (strcmp(msg->params, "sawd") == 0) {
      osc_num = add_osc(mixr, msg->freq, saw_down_table);
  } else if (strcmp(msg->params, "sawu") == 0) {
      osc_num = add_osc(mixr, msg->freq, saw_up_table);
  } else if (strcmp(msg->params, "tri") == 0) {
      osc_num = add_osc(mixr, msg->freq, tri_table);
  } else if (strcmp(msg->params, "square") == 0) {
      osc_num = add_osc(mixr, msg->freq, square_table);
  }

  faderrr(osc_num, UP);

  free(msg);
  return NULL;
}

void *fade_runrrr(void * arg)
{
  sbmsg *msg = arg;
  faderrr(msg->freq, DOWN); // really this is the osc_num

  return NULL;
}

void thrunner(sbmsg *msg)
{
  // need to ensure and free(msg) in all subtasks from here
  printf("Got CMD: %s\n", msg->cmd);
  pthread_t pthrrrd;
  if (strcmp(msg->cmd, "timed_sig_start") == 0) {
    if ( pthread_create (&pthrrrd, NULL, timed_sig_start, msg)) {
      fprintf(stderr, "Err, running phrrread..\n");
      return;
    }
  } else if (strcmp(msg->cmd, "faderrr") == 0) {
    if ( pthread_create (&pthrrrd, NULL, fade_runrrr, msg)) {
      fprintf(stderr, "Err, running phrrread..\n");
      return;
    }
  }
}


// void startrrr(int sig_num)
// {
//   double vol = mixr->signals[sig_num]->vol;
//   struct timespec ts;
//   ts.tv_sec = 0;
//   ts.tv_nsec = 5000000;
//   while (vol < 1.0) {
//     vol += 0.001;
//     volfunc(mixr->signals[sig_num], vol);
//     nanosleep(&ts, NULL);
//   }
//   volfunc(mixr->signals[sig_num], 1.0);
// }

void faderrr(int sig_num, direction d)
{
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 500000;
  double vol = 0;

  if (d == UP) {
    while (vol < 0.5) {
      vol += 0.0001;
      volfunc(mixr->signals[sig_num], vol);
      nanosleep(&ts, NULL);
    }
    volfunc(mixr->signals[sig_num], 0.5);
  } else {
    double vol = mixr->signals[sig_num]->vol;
    while (vol > 0.0) {
      vol -= 0.0001;
      volfunc(mixr->signals[sig_num], vol);
      nanosleep(&ts, NULL);
    }
    volfunc(mixr->signals[sig_num], 0.0);
  }
}
