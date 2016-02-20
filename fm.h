#ifndef FM_H
#define FM_H

#include "oscil.h"

typedef struct FM FM;

typedef struct FM {
  //OSC **oscillators;
  //int num_oscil;
  OSCIL* mod_osc;
  OSCIL* car_osc;
  float vol;

  float (*gen_next)(FM*);
} FM;

FM* new_fm(double modf, double carf);
float fm_gen_next(FM *fm);
void fm_status(FM *fm, char *status_string);
void mfm(FM *fm, char *osc, double val);

#endif
