#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>

#include "fm.h"
#include "oscil.h"
#include "sampler.h"
#include "sbmsg.h"
#include "sound_generator.h"
#include "table.h"

#define INITIAL_SIGNAL_SIZE 4

typedef struct t_mixer
{

  SOUNDGEN **sound_generators;
  int soundgen_num; // actual number of SGs
  int soundgen_size; // number of memory slots reserved for SGszz
  int delay_on;

  double volume;

} mixer;

typedef struct {
  mixer *mixr;
  float delay[(int) SAMPLE_RATE / 8];
  int delay_p;
  float left_phase;
  float right_phase;
} paData;

mixer *new_mixer(void);

void mixer_ps(mixer *mixr);
int add_osc(mixer *mixr, int freq, GTABLE *gt);
int add_fm(mixer *mixr, int ffreq, int cfreq);
int add_sample(mixer *mixr, char *filename, char *pattern);
int add_sound_generator(mixer *mixr, SBMSG *sbm);
void mixer_vol_change(mixer *mixr, float vol);
void vol_change(mixer *mixr, int sig, float vol);
void freq_change(mixer *mixr, int sig, float freq);
void delay_toggle(mixer *mixr);

double gen_next(mixer *mixr);
//void gen_next(mixer* mixr, int framesPerBuffer, float* out);

#endif // MIXER_H
