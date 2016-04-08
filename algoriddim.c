#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

extern pthread_cond_t bpm_cond;
extern pthread_mutex_t bpm_lock;

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

void play_melody(const int osc_num, int *mlock, int *note, int *notes, const int note_len)
{
  if (!*mlock) {

    *mlock = 1;
    *note = (*note + 1) % note_len;
    int randy = rand() % 100;
    if ( randy > 90 )
      freq_change(mixr, osc_num, (2 * notes[*note])); 
    else if ( randy < 10 )
      freq_change(mixr, osc_num, 0);
    else
      freq_change(mixr, osc_num, notes[*note]); 
  }
}

void fplay_melody(const int sg_num, int *mlock, int *note, int *notes, const int note_len)
{
  if (!*mlock) {

    *mlock = 1;
    *note = (*note + 1) % note_len;
    //freq_change(mixr, osc_num, (notes[*note])); 
    int randy = rand() % 100;
    if ( randy > 90 )
      mfm(mixr->sound_generators[sg_num], "car", (2 * notes[*note]));
    else
      mfm(mixr->sound_generators[sg_num], "car", (notes[*note]));
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

void *loop_run(void *m)
{
  melody_msg *mmsg = (melody_msg*) m;
  printf("LOOP RUN CALLED - got me a msg: %d, %d, %d\n", mmsg->melody[0], mmsg->melody[1], mmsg->melody_note_len);

  int osc_num = add_osc(mixr, mmsg->melody[0], sine_table);
  mmsg->osc_num = osc_num;
  do {} while (b->cur_tick % 16 != 0);
  faderrr(osc_num, UP);

  srand(time(NULL));

  while (1)
  {
    if (b->cur_tick % (4*(mmsg->melody_loop_len)) == 0) {
      play_melody(mmsg->osc_num, &mmsg->melody_play_lock, &mmsg->melody_cur_note, mmsg->melody, mmsg->melody_note_len);
    } else if (mmsg->melody_play_lock) {
      mmsg->melody_play_lock = 0;
    }
    pthread_mutex_lock(&bpm_lock);
    pthread_cond_wait(&bpm_cond,&bpm_lock);
    pthread_mutex_unlock(&bpm_lock);
  }
}

void *randdrum_run(void *m)
{
  SBMSG *msg = (SBMSG*) m;
  int drum_num = msg->sound_gen_num;
  int looplen = msg->looplen;
  printf("RANDRUN CALLED - got me a msg: drumnum %d - with length of %d bars\n", drum_num, looplen);
  int changed = 0;

  while (1) 
  {
    if (b->cur_tick % (4*looplen) == 0) {
        if ( !changed ) {
            changed = 1;
            int pattern = rand() % 65535; // max for an unsigned int
            //printf("My rand num %d\n", pattern);
            update_pattern(mixr->sound_generators[drum_num], pattern);
        }
    } else {
        changed = 0;
    }
    pthread_mutex_lock(&bpm_lock);
    pthread_cond_wait(&bpm_cond,&bpm_lock);
    pthread_mutex_unlock(&bpm_lock);
  }
}

void *floop_run(void *m)
{
  melody_msg *mmsg = (melody_msg*) m;
  printf("FLOOP RUN CALLED - got me a msg: %d, %d, %d MOD FREQ: %d\n", mmsg->melody[0], mmsg->melody[1], mmsg->melody_note_len, mmsg->mod_freq);

  int fm_one = add_fm(mixr, mmsg->mod_freq, mmsg->melody[0]);
  mmsg->osc_num = fm_one;
  do {} while (b->cur_tick % 16 != 0);
  faderrr(fm_one, UP);
  //sleep(3);
  srand(time(NULL));

  while (1)
  {
    if (b->cur_tick % (4*(mmsg->melody_loop_len)) == 0) {
      fplay_melody(mmsg->osc_num, &mmsg->melody_play_lock, &mmsg->melody_cur_note, mmsg->melody, mmsg->melody_note_len);
    } else if (mmsg->melody_play_lock) {
      mmsg->melody_play_lock = 0;
    }
    pthread_mutex_lock(&bpm_lock);
    pthread_cond_wait(&bpm_cond,&bpm_lock);
    pthread_mutex_unlock(&bpm_lock);
  }
}

void *algo_run(void *a)
{
  (void) a;

  int fm_one_lock = 0;
  // k_nuth
  //int notes[] = {311, 207, 466, 392, 261, 588, 466, 310, 699};
  int notes[] = {277, 184, 415};
  int notes_len = 3;

  int note = notes[rand() % notes_len];
  int fm_one = add_fm(mixr, 60, note);
  faderrr(fm_one, UP);

  //char *sigtype[2] = {"car", "mod"};
  //

  int note_index = 0;

  while (1)
  {
    if (b->cur_tick % (16) == 0)  {
      if (!fm_one_lock) {
        fm_one_lock = 1;
        //if (rand()%2) {
          int new_note = notes[note_index++];
          mfm(mixr->sound_generators[fm_one], "car", new_note);
          if (new_note >= notes_len)
            note_index = 0;
        //}
      }
    } else {
      fm_one_lock = 0;
    }
    pthread_mutex_lock(&bpm_lock);
    pthread_cond_wait(&bpm_cond,&bpm_lock);
    pthread_mutex_unlock(&bpm_lock);
  }
}
