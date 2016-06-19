#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <portaudio.h>

#include "bitwize.h"
#include "defjams.h"
#include "drumr.h"
#include "effect.h"
#include "envelope.h"
#include "fm.h"
#include "mixer.h"
#include "oscil.h"
#include "sampler.h"
#include "sbmsg.h"
#include "sound_generator.h"

extern ENVSTREAM* ampstream;

mixer *new_mixer()
{
  mixer *mixr = NULL;
  mixr = calloc(1, sizeof(mixer));
  mixr->volume = 0.7;
  mixr->keyboard_octave = 3;
  if (mixr == NULL) {
    printf("Nae mixer, fucked up!\n");
    return NULL;
  }
  return mixr;
}

void mixer_ps(mixer *mixr)
{
  printf(ANSI_COLOR_WHITE "::::: Mixing Desk (Volume: %f) (Delay On: %d) :::::\n" ANSI_COLOR_RESET, mixr->volume, mixr->delay_on);
  printf(ANSI_COLOR_GREEN "::::: effects: %d :::::\n" ANSI_COLOR_RESET, mixr->effects_num);
  for ( int i = 0; i < mixr->soundgen_num; i++) {
    char ss[120];
    mixr->sound_generators[i]->status(mixr->sound_generators[i], ss);
    printf("[%d] - %s\n", i, ss); 
  }
}

void delay_toggle(mixer *mixr)
{
  mixr->delay_on = abs(1 - mixr->delay_on);
  printf("MIXER VOL DELAY: %d!\n", mixr->delay_on);
}

void mixer_vol_change(mixer *mixr, float vol)
{
  printf("MIXER VOL CHANGE!\n");
  if (vol >= 0.0 && vol <= 1.0) {
    printf("PASSED THA CHALLEND WITH %F!\n", vol);
    mixr->volume = vol;
  }
}

void vol_change(mixer *mixr, int sg, float vol)
{
  if (sg > (mixr->soundgen_size - 1))
    return;
  mixr->sound_generators[sg]->setvol(mixr->sound_generators[sg], vol);
}

void freq_change(mixer *mixr, int sg, float freq)
{
  // TODO: safety check for OSC
  OSCIL *osc = (OSCIL *) mixr->sound_generators[sg];
  osc->freqadj(osc, freq);
}

int add_effect(mixer *mixr)
{
  printf("Booya, adding a new effect!\n");
  EFFECT **new_effects = NULL;
  if (mixr->effects_size <= mixr->effects_num) {
    if (mixr->effects_size == 0) {
      mixr->effects_size = DEFAULT_ARRAY_SIZE;
    } else {
    mixr->effects_size *= 2;
    }

    new_effects = realloc(mixr->effects, mixr->effects_size *
                      sizeof(EFFECT*));
    if (new_effects == NULL) {
      printf("Ooh, burney - cannae allocate memory for new sounds");
      return -1;
    } else {
      mixr->effects = new_effects;
    }
  }

  EFFECT* e = new_delay(0.2, DELAY);
  if ( e == NULL ) {
    perror("Couldn't create effect");
    return -1;
  }
  mixr->effects[mixr->effects_num] = e;
  printf("done adding effect\n");
  return mixr->effects_num++;
}

int add_sound_generator(mixer *mixr, SBMSG *sbm)
{
  SOUNDGEN **new_soundgens = NULL;
  if (mixr->soundgen_size <= mixr->soundgen_num) {
    if (mixr->soundgen_size == 0) {
      mixr->soundgen_size = DEFAULT_ARRAY_SIZE;
    } else {
    mixr->soundgen_size *= 2;
    }

    new_soundgens = realloc(mixr->sound_generators, mixr->soundgen_size *
                      sizeof(SOUNDGEN*));
    if (new_soundgens == NULL) {
      printf("Ooh, burney - cannae allocate memory for new sounds");
      return -1;
    } else {
      mixr->sound_generators = new_soundgens;
    }
  }
  mixr->sound_generators[mixr->soundgen_num] = sbm->sound_generator;
  return mixr->soundgen_num++;
}

int add_bitwize(mixer *mixr, int pattern)
{

  BITWIZE *new_bitw = new_bitwize(pattern);
  if (new_bitw == NULL) {
    printf("BITBARF!\n");
    return -1;
  }

  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    free(new_bitw);
    printf("MBITBARF!\n");
    return -1;
  }

  m->sound_generator = (SOUNDGEN *) new_bitw;
  printf("Added bitwize gen!\n");
  return add_sound_generator(mixr, m);
}
int add_osc(mixer *mixr, double freq, GTABLE *gt)
{

  OSCIL *new_osc = new_oscil(freq, gt);
  if (new_osc == NULL) {
    printf("BARF!\n");
    return -1;
  }

  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    free(new_osc);
    printf("MBARF!\n");
    return -1;
  }

  m->sound_generator = (SOUNDGEN *) new_osc;
  m->freq = 1492;
  printf("Added sound gen!\n");
  return add_sound_generator(mixr, m);
}

int add_fm_x(mixer *mixr, char* f_osc, double ffreq, char* c_osc, double cfreq)
{
  FM *nfm = new_fm_x(f_osc, ffreq, c_osc, cfreq);
  if (nfm == NULL) {
    printf("Barfed on FM creation\n");
    return -1;
  }
  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    free(nfm);
    printf("MBARF!\n");
    return -1;
  }
  m->sound_generator = (SOUNDGEN *) nfm;
  return add_sound_generator(mixr, m);
}

int add_fm(mixer *mixr, double ffreq, double cfreq)
{
  FM *nfm = new_fm(ffreq, cfreq);
  if (nfm == NULL) {
    printf("Barfed on FM creation\n");
    return -1;
  }
  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    free(nfm);
    printf("MBARF!\n");
    return -1;
  }
  m->sound_generator = (SOUNDGEN *) nfm;
  return add_sound_generator(mixr, m);
}

int add_drum(mixer *mixr, char *filename, char *pattern)
{
  // preliminary setup
  char cwd[1024];
  getcwd(cwd, 1024);
  char full_filename[strlen(filename) + strlen(cwd) + 7]; // 7 == '/wavs/' is 6 and 1 for '\0'
  strcpy(full_filename, cwd);
  strcat(full_filename, "/wavs/");
  strcat(full_filename, filename);

  DRUM *ndrum = new_drumr(full_filename, pattern);
  if (ndrum == NULL) {
    printf("Barfed on drum creation\n");
    return -1;
  }
  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    free(ndrum);
    printf("MBARF!\n");
    return -1;
  }

  m->sound_generator = (SOUNDGEN *) ndrum;
  return add_sound_generator(mixr, m);
}

int add_sampler(mixer *mixr, char *filename, double loop_len)
{
  // preliminary setup
  char cwd[1024];
  getcwd(cwd, 1024);
  char full_filename[strlen(filename) + strlen(cwd) + 7]; // 7 == '/wavs/' is 6 and 1 for '\0'
  strcpy(full_filename, cwd);
  strcat(full_filename, "/wavs/");
  strcat(full_filename, filename);

  printf("ADD SAMPLER - LOOP LEN %f\n", loop_len);
  SAMPLER *nsampler = new_sampler(full_filename, loop_len);
  if (nsampler == NULL) {
    printf("Barfed on sampler creation\n");
    return -1;
  }
  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    free(nsampler);
    printf("SAMPLMBARF!\n");
    return -1;
  }

  m->sound_generator = (SOUNDGEN *) nsampler;
  return add_sound_generator(mixr, m);
}
//void gen_next(mixer* mixr, int framesPerBuffer, float* out)
double gen_next(mixer* mixr)
{
  double output_val = 0.0;
  if (mixr->soundgen_num > 0) {
    for (int i = 0; i < mixr->soundgen_num; i++) {
      output_val += mixr->sound_generators[i]->gennext(mixr->sound_generators[i]);
    }
  }

  // global envelope -- for the moment
  //double mix_amp = envelope_stream_tick(ampstream);
  //output_val *= mix_amp;
  return mixr->volume * (output_val / 1.53);
}
