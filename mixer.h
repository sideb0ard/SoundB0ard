#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>

#include "oscil.h"
#include "oscilt.h"

#define INITIAL_SIGNAL_SIZE 4

typedef struct t_mixer
{
  OSCIL **signals;
  int num_sig; // actual number of signals
  int sig_size; // number of memory slots referenced for sigs;

  OSCILT **tsignals;
  int num_tsig; // actual number of signals
  int tsig_size; // number of memory slots referenced for sigs;

  double volume;
} mixer;

mixer *new_mixer(void);
void *mixer_run(void *);
void mixer_ps(mixer *mixr);
double gen_next(mixer *mixr);
void add_osc(mixer *mixr, uint32_t freq, tickfunc tic);
void add_tosc(mixer *mixr, uint32_t freq, ttickfunc tic);

#endif // MIXER_H
