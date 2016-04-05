#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <regex.h>
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
  } else if (strcmp(msg->params, "sloop") == 0) {
      sg = add_sampler(mixr, msg->filename, msg->looplen);
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

void *duck_runrrr(void * arg)
{
  SBMSG *msg = arg;
  printf("Duckin' %d\n", msg->sound_gen_num);
  faderrr(msg->sound_gen_num, DOWN);
  sleep( rand() % 15);
  faderrr(msg->sound_gen_num, UP);

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
  } else if (strcmp(msg->cmd, "duckrrr") == 0) {
    if ( pthread_create (&pthrrrd, NULL, duck_runrrr, msg)) {
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
    double vol = mixr->sound_generators[sg_num]->getvol(mixr->sound_generators[sg_num]);
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
  char *ap, *ap_last, *fargv[8];
  int freq_count = 0;
  char *sep = " ";

  for ( ap = strtok_r(string, sep, &ap_last);
        ap;
        ap = strtok_r(NULL, sep, &ap_last))
  {
      fargv[freq_count++] = ap;
  }

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

int notelookup(char *n)
{
  // twelve semitones:
  // C C#/Db D D#/Eb E F F#/Gb G G#/Ab A A#/Bb B
  //
  if (!strcasecmp("c", n)) return 0;
  else if (!strcasecmp("c#", n)) return 1;
  else if (!strcasecmp("db", n)) return 1;
  else if (!strcasecmp("d",  n)) return 2;
  else if (!strcasecmp("d#", n)) return 3;
  else if (!strcasecmp("eb", n)) return 3;
  else if (!strcasecmp("e",  n)) return 4;
  else if (!strcasecmp("f",  n)) return 5;
  else if (!strcasecmp("f#", n)) return 6;
  else if (!strcasecmp("gb", n)) return 6;
  else if (!strcasecmp("g",  n)) return 7;
  else if (!strcasecmp("g#", n)) return 8;
  else if (!strcasecmp("ab", n)) return 8;
  else if (!strcasecmp("a",  n)) return 9;
  else if (!strcasecmp("a#", n)) return 10;
  else if (!strcasecmp("bb", n)) return 10;
  else if (!strcasecmp("b",  n)) return 11;
  else
    return -1;
}

float freqval(char *n)
{
  // algo from http://www.phy.mtu.edu/~suits/NoteFreqCalcs.html
  float a4 = 440.0; // fixed note freq to use as baseline
  const float twelfth_root_of_two = 1.059463094359;

  regmatch_t nmatch[3];
  regex_t single_letter_rx;
  regcomp(&single_letter_rx, "^([[:alpha:]#]{1,2})([[:digit:]])$", REG_EXTENDED|REG_ICASE);
  if (regexec(&single_letter_rx, n, 3, nmatch, 0) == 0) {

    int note_str_len = nmatch[1].rm_eo - nmatch[1].rm_so;
    char note[note_str_len + 1];
    strncpy(note, n+nmatch[1].rm_so, note_str_len);
    note[note_str_len] = '\0';

    char str_octave[2];
    strncpy(str_octave, n+note_str_len, 1);
    str_octave[1] = '\0';

    // purpose of this is working out how many semitones the given note is from A4
    int n_num = (12 * atoi(str_octave)) + notelookup(note);
    // fixed note, which we compare against is A4 - '4' is the fourth
    // octave, so 4 * 12 semitones, plus lookup val of A is '9' - so 57
    int diff = n_num - 57;

    float freqval = a4 * (pow(twelfth_root_of_two, diff));
    return freqval;
  } else {
    return -1.0;
  }
}

void strim(const char *input, char *result)
{
  int flag = 0;

  while(*input) {
    if (!isspace((unsigned char) *input) && flag == 0) {
      *result++ = *input;
      flag = 1;
    }
    input++;
    if (flag == 1) {
      *result++ = *input;
    }
  }

  while (1) {
    result--;
    if (!isspace((unsigned char) *input) && flag == 0) {
      break;
    }
    flag = 0;
    *result = '\0';
  }
}
