#ifndef SAMPLER_H
#define SAMPLER_H

#include "sound_generator.h"

#define SAMPLE_PATTERN_LEN 16

typedef struct t_sampler SAMPLER;
// typedef double (*fp_gennext) (SAMPLER* sampler);

typedef struct t_sampler
{

  SOUNDGEN sound_generator;

  char *filename;
  int pattern[SAMPLE_PATTERN_LEN];
  int *buffer;
  int bufsize;
  int position;
  int playing;
  int played;
  int samplerate;
  int channels;
  double vol;

  // fp_gennext gen_next;

} SAMPLER;

SAMPLER* new_sampler(char *filename, char *pattern);

double sample_gennext(void *self);
void sample_status(void *self, char *ss);

#endif // SAMPLER_H
