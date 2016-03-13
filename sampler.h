#ifndef SAMPLER_H
#define SAMPLER_H

#include "sound_generator.h"

typedef struct t_sampler
{
  SOUNDGEN sound_generator;
  char *filename;
  int *buffer;
  int bufsize;
  int position;
  int playing;
  int played;
  int samplerate;
  int channels;
  int loop_len;
  double vol;
  int incr;
  int curtick;
} SAMPLER;

SAMPLER* new_sampler(char *filename, int loop_len); // loop_len in bars

//void sampler_gennext(void* self, double* frame_vals, int framesPerBuffer);
double sampler_gennext(void* self);
void sampler_status(void *self, char *ss);
void sampler_setvol(void *self, double v);
double sampler_getvol(void *self);
void sampler_set_incr(void *self);

#endif // SAMPLER_H
