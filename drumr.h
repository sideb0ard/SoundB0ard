#ifndef DRUM_H
#define DRUM_H

#include "sound_generator.h"

#define DRUM_PATTERN_LEN 16

typedef struct t_drumr
{
  SOUNDGEN sound_generator;
  char *filename;
  // int pattern[DRUM_PATTERN_LEN];
  int pattern; // bitmask version!
  int *buffer;
  int bufsize;
  int position;
  int playing;
  int played;
  int tick;
  int samplerate;
  int channels;
  double vol;
} DRUM;

DRUM* new_drumr(char *filename, char *pattern);

//void drum_gennext(void* self, double* frame_vals, int framesPerBuffer);
void drum_status(void *self, char *ss);
void drum_setvol(void *self, double v);
double drum_gennext(void* self);
double drum_getvol(void *self);
void update_pattern(void *self, int newpattern);

#endif // DRUM_H
