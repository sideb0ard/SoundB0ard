#ifndef MIXER_H
#define MIXER_H

#include "wave.h"

#define INITIAL_SIGNAL_SIZE 4

typedef struct t_mixer
{
  OSCIL **signals;
  int num_sig; // actual number of signals
  int sig_size; // number of memory slots referenced for sigs;
  double volume;
} mixer;

mixer *new_mixer();
void *mixer_run(void *);
void mixer_ps(mixer *mixr);
double gen_next(mixer *mixr);
void add_osc(mixer *mixr, uint32_t freq);

#endif // MIXER_H
