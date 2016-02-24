#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "sampler.h"

extern bpmrrr *b;

SAMPLER* new_sampler(char *filename, char *pattern)
{
  SAMPLER *sampler = calloc(1, sizeof(SAMPLER));
  sampler->position = 0;

  // sample pattern stuff
  char **sp, *spattern[32];
  int sp_count = 0;

  for (sp = spattern; (*sp = strsep(&pattern, " ")) != NULL;) {
    sp_count++;
    if (**sp != '\0')
      if (++sp >= &spattern[32])
        break;
  }

  for (int i = 0; i < sp_count; i++) {
    int pat_num = atoi(spattern[i]);
    if (pat_num < SAMPLE_PATTERN_LEN) {
      sampler->pattern[pat_num] = 1;
    }
  }

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
    printf("Ooft, memory issues, mate!\n");
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
  sampler->vol = 0.7;

  sampler->sound_generator.gennext = &sample_gennext;
  sampler->sound_generator.status = &sample_status;
  sampler->sound_generator.getvol = &sample_getvol;
  sampler->sound_generator.setvol = &sample_setvol;

  return sampler;
}

double sample_gennext(void *self)
{
  SAMPLER *sampler = self;
  static double val;

  if (sampler->pattern[b->cur_tick % SAMPLE_PATTERN_LEN]) {
    if (!sampler->played) {
      sampler->playing = 1;
    }
    if (sampler->playing) {
      val =  sampler->buffer[sampler->position++] / 2147483648.0 ; // convert from 16bit in to double between 0 and 1
    } else {
      val = 0.0;
    }
    if ((int)sampler->position == sampler->bufsize) { // end of playback - so reset
      sampler->played = 1;
      sampler->playing = 0;
      sampler->position = 0;
    }
  } else {
    sampler->position = 0;
    sampler->played = 0;
  }

  return val * sampler->vol;
  
}

void sample_status(void *self, char *status_string)
{
  SAMPLER *sampler = self;
  char spattern[SAMPLE_PATTERN_LEN + 1] = "";
  for (int i = 0; i < SAMPLE_PATTERN_LEN; i++) {
    if (sampler->pattern[i]) {
      strncat(&spattern[i], "1", 1);
    } else {
      strncat(&spattern[i], "0", 1);
    }
  }
  spattern[SAMPLE_PATTERN_LEN] = '\0';
  snprintf(status_string, 119, ANSI_COLOR_CYAN "[%s]\t[%s] vol: %.2lf" ANSI_COLOR_RESET, sampler->filename, spattern, sampler->vol);
}

double sample_getvol(void *self)
{
  SAMPLER *sampler = self;
  return sampler->vol;
}

void sample_setvol(void *self, double v)
{
  SAMPLER *sampler = self;
  if (v < 0.0 || v > 1.0) {
    return;
  }
  sampler->vol = v;
}
