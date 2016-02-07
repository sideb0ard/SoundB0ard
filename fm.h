#ifndef FM_H
#define FM_H

#include "oscil.h"

typedef struct FM FM;

typedef struct FM {
  //OSC **oscillators;
  //int num_oscil;
  OSCIL* fmod_osc;
  OSCIL* cmod_osc;
  float vol;

  float (*gen_next)(FM*);
} FM;

FM* new_fm(double fmod, double cmod);
float fm_gen_next(FM *fm);
void fm_status(FM *fm);

#endif
