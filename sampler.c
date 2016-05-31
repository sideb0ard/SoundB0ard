#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "sampler.h"

extern bpmrrr *b;

SAMPLER* new_sampler(char *filename, double loop_len)
{
  SAMPLER *sampler = calloc(1, sizeof(SAMPLER));
  sampler->position = 0;

  // soundfile part
  SNDFILE *snd_file;
  SF_INFO sf_info;

  sf_info.format = 0;
  snd_file = sf_open(filename, SFM_READ, &sf_info);
  if (!snd_file) {
    printf("Err opening %s : %d\n", filename, sf_error(snd_file));
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
    perror("Ooft, memory issues, mate!\n");
    return (void*) NULL;
  }

  sf_readf_int(snd_file, buffer, bufsize);

  int fslen = strlen(filename);
  sampler->filename = calloc(1, fslen + 1);
  strncpy(sampler->filename, filename, fslen);

  sampler->buffer = buffer;
  sampler->bufsize = bufsize;
  sampler->samplerate = sf_info.samplerate;
  sampler->channels = sf_info.channels;
  printf("LOOP LEN IS %f\n", loop_len);
  sampler->loop_len = loop_len;
  printf("SAMPLER LOOP LEN IS %f\n", sampler->loop_len);
  sampler->vol = 0.0;

  sampler_set_incr(sampler);

  sampler->sound_generator.gennext = &sampler_gennext;
  sampler->sound_generator.status = &sampler_status;
  sampler->sound_generator.getvol = &sampler_getvol;
  sampler->sound_generator.setvol = &sampler_setvol;
  sampler->sound_generator.type = SAMPLER_TYPE;

  return sampler;
}

void sampler_set_incr(void* self) 
{
  SAMPLER *sampler = self;
  printf("BUFSIZE is %d\n", sampler->bufsize);
  double incr = sampler->bufsize / (SAMPLE_RATE * (60.0 / b->bpm * 4) / sampler->loop_len * sampler->channels);
  printf("INCR is %f\n", incr);
  sampler->incr = incr;
}

double sampler_gennext(void *self)
//void sampler_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
  SAMPLER *sampler = self;
  //double val = 0;

  int base_index = (int) (sampler->position);
  unsigned long next_index = base_index + 1;
  double frac, slope, val;
  double bufsize = sampler->bufsize, position = sampler->position;
  int* table = sampler->buffer;
 
  frac = position - base_index;
  val = table[base_index];
  slope = table[next_index] - val;

  val += (frac * slope);
  position += sampler->incr;

  while ( position >= bufsize )
    position -= bufsize;
  while ( position < 0.0 )
    position += bufsize;

  sampler->position = position;
  //val =  sampler->buffer[sampler->position] / 2147483648.0 ; // convert from 16bit in to double between 0 and 1

  val = val / 2147483648.0 ;

  if (val > 1 || val < -1)
    printf("BURNIE - SAMPLER OVERLOAD!\n");

  val = effector(&sampler->sound_generator, val);
  val = envelopor(&sampler->sound_generator, val);

  return val * sampler->vol;
}

void sampler_status(void *self, char *status_string)
{
  SAMPLER *sampler = self;
  snprintf(status_string, 119, COOL_COLOR_GREEN "[%s]\tvol: %.2lf" ANSI_COLOR_RESET, basename(sampler->filename), sampler->vol);
}

double sampler_getvol(void *self)
{
  SAMPLER *sampler = self;
  return sampler->vol;
}

void sampler_setvol(void *self, double v)
{
  SAMPLER *sampler = self;
  if (v < 0.0 || v > 1.0) {
    return;
  }
  sampler->vol = v;
}
