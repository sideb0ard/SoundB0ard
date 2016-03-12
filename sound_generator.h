#ifndef SOUNDGEN_H
#define SOUNDGEN_H

#include "defjams.h"

//typedef enum 
//{
//  OSC,
//  FM,
//  SAMPLER
//} sound_generator_type;
//

typedef struct t_soundgen {
  //void (*gennext)(void *self, double* frame_vals, int framesPerBuffer);
  double (*gennext)(void *self);
  void (*status)(void *self, char *string);
  void (*setvol)(void *self, double val);
  double (*getvol)(void *self);
  //sound_generator_type type;

  int effects_size; // size of array
  int effects_num; // num of effects
  int *effects;
  int effects_on; // bool
} SOUNDGEN;

void link_effect(SOUNDGEN* sg, int effect_no);

#endif // SOUNDGEN_H
