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
      data->pattern[pat_num] = 1;
    }
  }

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
  data->filename = calloc(1, fslen + 1);
  strncpy(data->filename, filename, fslen);

  data->buffer = buffer;
  data->bufsize = bufsize;
  data->samplerate = sf_info.samplerate;
  data->channels = sf_info.channels;
  data->gen_next = &f_gennext;
  data->vol = 0.7;

  return data;
}

double f_gennext(SAMPLER *sampler)
{
  static double val;

  if (sampler->pattern[b->cur_tick % SAMPLE_PATTERN_LEN]) {
    if (!sampler->playing) {
      val =  sampler->buffer[sampler->position++] / 2147483648.0 ; // convert from 16bit in to double between 0 and 1
    } else {
      val = 0.0;
    }
    if ((int)sampler->position == sampler->bufsize) { // end of playback - so reset
      sampler->position = 0;
      sampler->playing = 0;
    }
  } else {
    sampler->position = 0;
    sampler->playing = 0;
  }

  return val * sampler->vol;
  
}

void sample_status(SAMPLER *sampler, char *status_string)
{
  char spattern[SAMPLE_PATTERN_LEN + 1] = "";
  for (int i = 0; i < SAMPLE_PATTERN_LEN; i++) {
    if (sampler->pattern[i]) {
      strncat(&spattern[i], "1", 1);
    } else {
      strncat(&spattern[i], "0", 1);
    }
  }
  spattern[SAMPLE_PATTERN_LEN] = '\0';
  snprintf(status_string, 80, "[%s]\t[%s]", sampler->filename, spattern);
}


