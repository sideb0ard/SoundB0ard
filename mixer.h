#ifndef MIXER_H
#define MIXER_H

#include "wave.h"

typedef struct t_mixer
{
  OSCIL **signals;
  double volume;
} mixer;

mixer *new_mixer();
void *mixer_run(void *);

#endif // MIXER_H
