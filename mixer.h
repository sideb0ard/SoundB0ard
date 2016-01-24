#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>

#include "oscil.h"
#include "table.h"

#define INITIAL_SIGNAL_SIZE 4

typedef struct t_mixer
{
  OSCIL **signals;
  int num_sig; // actual number of signals
  int sig_size; // number of memory slots referenced for sigs;

  double volume;

} mixer;

mixer *new_mixer(void);

void *mixer_run(void *); // TODO: need this?
void mixer_ps(mixer *mixr);
void add_osc(mixer *mixr, int freq, GTABLE *gt);

double gen_next(mixer *mixr);

#endif // MIXER_H
