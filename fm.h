#ifndef FM_H
#define FM_H

#include "oscil.h"
#include "sound_generator.h"

typedef struct FM FM;

typedef struct FM {
  //OSC **oscillators;
  //int num_oscil;
  SOUNDGEN sound_generator;

  OSCIL* mod_osc;
  OSCIL* car_osc;
  float vol;

  //float (*gen_next)(FM*);
} FM;

FM* new_fm(double modf, double carf);

double fm_gennext(void *self);
void fm_status(void *self, char *status_string);
void mfm(FM *fm, char *osc, double val);

#endif
