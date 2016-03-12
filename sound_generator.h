#ifndef SOUNDGEN_H
#define SOUNDGEN_H

#include "effect.h"
#include "envelope.h"
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
  EFFECT **effects;
  int effects_on; // bool

  int envelopes_size; // size of array
  int envelopes_num; // num of effects
  ENVSTREAM **envelopes;
  int envelopes_on; // bool

} SOUNDGEN;

int add_effect_soundgen(SOUNDGEN* self, float duration);
float effector(SOUNDGEN* self, float val);

int add_envelope_soundgen(SOUNDGEN* self, int env_len);
float envelopor(SOUNDGEN* self, float val);

#endif // SOUNDGEN_H
