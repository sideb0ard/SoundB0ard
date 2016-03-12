#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

#include "bpmrrr.h"
#include "defjams.h"
#include "drumr.h"

extern bpmrrr *b;

DRUM* new_drumr(char *filename, char *pattern)
{
  DRUM *drumr = calloc(1, sizeof(DRUM));
  drumr->position = 0;

  // drum pattern stuff
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
    if (pat_num < drum_PATTERN_LEN) {
      drumr->pattern[pat_num] = 1;
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
  drumr->filename = calloc(1, fslen + 1);
  strncpy(drumr->filename, filename, fslen);

  drumr->buffer = buffer;
  drumr->bufsize = bufsize;
  drumr->samplerate = sf_info.samplerate;
  drumr->channels = sf_info.channels;
  drumr->vol = 0.7;

  drumr->sound_generator.gennext = &drum_gennext;
  drumr->sound_generator.status = &drum_status;
  drumr->sound_generator.getvol = &drum_getvol;
  drumr->sound_generator.setvol = &drum_setvol;

  return drumr;
}

double drum_gennext(void *self)
//void drum_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
  DRUM *drumr = self;
  double val = 0;

  if (drumr->pattern[b->cur_tick % drum_PATTERN_LEN]) {
    if (!drumr->played) {
      drumr->playing = 1;
    }
    if (drumr->playing) {
      val =  drumr->buffer[drumr->position++] / 2147483648.0 ; // convert from 16bit in to double between 0 and 1
    } else {
      val = 0.0;
    }
    if ((int)drumr->position == drumr->bufsize) { // end of playback - so reset
      drumr->played = 1;
      drumr->playing = 0;
      drumr->position = 0;
    }
  } else {
    drumr->position = 0;
    drumr->played = 0;
  }
  if (val > 1 || val < -1)
    printf("BURNIE - DRUM OVERLOAD!\n");

  val = effector(&drumr->sound_generator, val);
  val = envelopor(&drumr->sound_generator, val);

  return val * drumr->vol;
}

void drum_status(void *self, char *status_string)
{
  DRUM *drumr = self;
  char spattern[drum_PATTERN_LEN + 1] = "";
  for (int i = 0; i < drum_PATTERN_LEN; i++) {
    if (drumr->pattern[i]) {
      strncat(&spattern[i], "1", 1);
    } else {
      strncat(&spattern[i], "0", 1);
    }
  }
  spattern[drum_PATTERN_LEN] = '\0';
  snprintf(status_string, 119, ANSI_COLOR_CYAN "[%s]\t[%s] vol: %.2lf" ANSI_COLOR_RESET, drumr->filename, spattern, drumr->vol);
}

double drum_getvol(void *self)
{
  DRUM *drumr = self;
  return drumr->vol;
}

void drum_setvol(void *self, double v)
{
  DRUM *drumr = self;
  if (v < 0.0 || v > 1.0) {
    return;
  }
  drumr->vol = v;
}
