#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
  SBMSG *msg = arg;
  int sg = -1; // signal generator

  if (strcmp(msg->params, "sine") == 0) {
      sg = add_osc(mixr, msg->freq, sine_table);
  } else if (strcmp(msg->params, "sawd") == 0) {
      sg = add_osc(mixr, msg->freq, saw_down_table);
  } else if (strcmp(msg->params, "sawu") == 0) {
      sg = add_osc(mixr, msg->freq, saw_up_table);
  } else if (strcmp(msg->params, "tri") == 0) {
      sg = add_osc(mixr, msg->freq, tri_table);
  } else if (strcmp(msg->params, "square") == 0) {
      sg = add_osc(mixr, msg->freq, square_table);
  } else if (strcmp(msg->params, "fm") == 0) {
      sg = add_fm(mixr, msg->modfreq, msg->carfreq);
  }

  faderrr(sg, UP);

  free(msg);
  return NULL;
}

void *fade_runrrr(void * arg)
{
  SBMSG *msg = arg;
  faderrr(msg->sound_gen_num, DOWN);

  return NULL;
}

void thrunner(SBMSG *msg)
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

void faderrr(int sg_num, direction d)
{

  printf("FADING CALLED: GOING %d!\n", d);
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 500000;
  double vol = 0;

  if (d == UP) {
    while (vol < 0.7) {
      vol += 0.0001;
      mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], vol);
      nanosleep(&ts, NULL);
    }
    mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], 0.7);
  } else {
    printf("GOING DOWN for %d\n", sg_num);
    double vol = mixr->sound_generators[sg_num]->getvol(mixr->sound_generators[sg_num]);
    printf("STARTING VOL, GOING DOWN: %lf\n", vol);
    while (vol > 0.0) {
      vol -= 0.0001;
      mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], vol);
      nanosleep(&ts, NULL);
    }
    mixr->sound_generators[sg_num]->setvol(mixr->sound_generators[sg_num], 0.0);
  }
}

freaky* new_freqs_from_string(char* string)
{
  //char * freaks = strdup(string);

  char **ap, *fargv[8];
  int freq_count = 0;
  //for (ap = fargv; (*ap = strsep(&freaks, " ")) != NULL;) {
  for (ap = fargv; (*ap = strsep(&string, " ")) != NULL;) {
    freq_count++;
    if (**ap != '\0')
      if (++ap >= &fargv[8])
        break;
  }
  //free(freaks);

  freaky* f = calloc(1, sizeof(freaky));
  f->num_freaks = freq_count;
  f->freaks = calloc(freq_count, sizeof(int));

  for (int i = 0; i < freq_count; i++) {
    f->freaks[i] = atoi(fargv[i]);
  }
  return f;
}

void list_sample_dir()
{
  DIR *dp;
  struct dirent *ep;
  dp = opendir("./wavs");
  if (dp != NULL) {
    while ((ep = readdir (dp)))
      puts (ep->d_name);
    (void) closedir (dp);
  } else {
    perror("Couldn't open wavs dir\n");
  }
}

