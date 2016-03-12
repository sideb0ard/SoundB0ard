#ifndef DRUM_H
#define DRUM_H

#include "sound_generator.h"

#define drum_PATTERN_LEN 16

typedef struct t_drumr
{
  SOUNDGEN sound_generator;
  char *filename;
  int pattern[drum_PATTERN_LEN];
  int *buffer;
  int bufsize;
  int position;
  int playing;
  int played;
  int samplerate;
  int channels;
  double vol;
} DRUM;

DRUM* new_drumr(char *filename, char *pattern);

//void drum_gennext(void* self, double* frame_vals, int framesPerBuffer);
double drum_gennext(void* self);
void drum_status(void *self, char *ss);
void drum_setvol(void *self, double v);
double drum_getvol(void *self);

#endif // DRUM_H
