#include <sndfile.h>
#include <stdlib.h>

#include "bpmrrr.h"
#include "sampler.h"

extern bpmrrr *b;

SAMPLER* new_sampler(char * filename)
{
  SAMPLER *data = calloc(1, sizeof(SAMPLER));
  data->position = 0;

  SNDFILE *snd_file;
  SF_INFO sf_info;

  sf_info.format = 0;
  snd_file = sf_open(filename, SFM_READ, &sf_info);
  if (!snd_file) {
    printf("Err opening file mate\n");
    exit(-1);
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
  data->pattern[0] = 1;
  data->pattern[4] = 1;
  data->pattern[8] = 1;
  data->pattern[16] = 1;
  data->pattern[24] = 1;
  data->pattern[28] = 1;

  return data;
}

double f_gennext(SAMPLER *sampler)
{
  //double val = 0.0;
  double val = sampler->buffer[sampler->position++];

  if (sampler->pattern[b->cur_tick % 32]) {
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



