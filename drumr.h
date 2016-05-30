#ifndef DRUM_H
#define DRUM_H

#include "sound_generator.h"

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
  int tick;
  int swing;
  int swing_setting;
  int tick_started;
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
void swingrrr(void *self, int swing_setting);

#endif // DRUM_H
