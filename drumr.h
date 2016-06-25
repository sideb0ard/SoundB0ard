#ifndef DRUM_H
#define DRUM_H

#include <sndfile.h>
#include "sound_generator.h"

typedef struct t_sample_pos 
{
    int position;
    int playing;
    int played;
} sample_pos;

typedef struct t_drumr
{
  SOUNDGEN sound_generator;
  sample_pos sample_positions[DRUM_PATTERN_LEN];
  char *filename;
  int pattern; // bitmask version!
  int *patterns;
  int num_patterns;
  int *buffer;
  int bufsize;
  //int position;
  //int playing;
  //int played;
  int tick;
  int swing;
  int swing_setting;
  // int tick_started;
  int samplerate;
  int channels;
  double vol;
} DRUM;


DRUM* new_drumr(char *filename, char *pattern);

//void drum_gennext(void* self, double* frame_vals, int framesPerBuffer);
void    drum_status(void *self, char *ss);
void    drum_setvol(void *self, double v);
void    update_pattern(void *self, int newpattern);
void    swingrrr(void *self, int swing_setting);
void    pattern_char_to_int(char* chpattern, int *pattern);
int*    load_file_to_buffer(char *filename, int *bufsize, SF_INFO *sf_info);

double  drum_gennext(void* self);
double  drum_getvol(void *self);

void*   randdrum_run(void *m);


#endif // DRUM_H
