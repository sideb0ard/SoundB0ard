#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include "bpmrrr.h"
#include "sampler.h"

extern bpmrrr *b;

SAMPLER* new_sampler(char *filename, char *pattern)
{
  SAMPLER *data = calloc(1, sizeof(SAMPLER));
  data->position = 0;

  printf("Gonna open %s\n", filename);
  printf("My pattern is %s\n", pattern);
  
  char **sp, *spattern[32];
  int sp_count = 0;
  //for (ap = fargv; (*ap = strsep(&freaks, " ")) != NULL;) {
  for (sp = spattern; (*sp = strsep(&pattern, " ")) != NULL;) {
    sp_count++;
    if (**sp != '\0')
      if (++sp >= &spattern[32])
        break;
  }
  for (int i = 0; i < sp_count; i++) {
    int pat_num = atoi(spattern[i]);
    if (pat_num < SAMPLE_PATTERN_LEN) {
      printf("PATTERNNNN: %d\n", pat_num);
      data->pattern[pat_num] = 1;
    }
  }

  SNDFILE *snd_file;
  SF_INFO sf_info;

  sf_info.format = 0;
  snd_file = sf_open(filename, SFM_READ, &sf_info);
  if (!snd_file) {
    printf("Err opening file mate : %d\n", sf_error(snd_file));
    return (void*) NULL;
  }
  printf("Filename:: %s\n", filename);
  printf("SR: %d\n", sf_info.samplerate);
  printf("Channels: %d\n", sf_info.channels);
  printf("Frames: %lld\n", sf_info.frames);
  int bufsize = sf_info.channels * sf_info.frames;
  printf("Making buffer size of %d\n", bufsize);

  int *buffer = calloc(bufsize, sizeof(int));
  if (buffer == NULL) {
    printf("Ooft, memory issues, mate!\n");
    return (void*) NULL;
  }

  sf_readf_int(snd_file, buffer, bufsize);
  data->buffer = buffer;
  data->bufsize = bufsize;
  data->samplerate = sf_info.samplerate;
  data->channels = sf_info.channels;
  data->gen_next = &f_gennext;
  data->vol = 0.7;
  //data->pattern[0] = 1;
  //data->pattern[5] = 1;
  //data->pattern[9] = 1;
  //data->pattern[13] = 1;

  printf("Returning, yo!\n");
  return data;
}

double f_gennext(SAMPLER *sampler)
{
  //double val = 0.0;
  double val = sampler->buffer[sampler->position++];

  if (sampler->pattern[b->cur_tick % 8]) {
    sampler->playing = 1;
  }

  if (sampler->playing) {
    val =  val / 2147483648.0 ; // convert from 16bit in to double between 0 and 1
  } else {
    val = 0.0;
  }

  if ((int)sampler->position == sampler->bufsize) { // end of playback - so reset
    sampler->position = 0;
    sampler->playing = 0;
  }

  return val * sampler->vol;
  
}



