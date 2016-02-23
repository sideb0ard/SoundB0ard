#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <portaudio.h>

#include "defjams.h"
#include "envelope.h"
#include "fm.h"
#include "mixer.h"
#include "oscil.h"
#include "sbmsg.h"
#include "sampler.h"
#include "sound_generator.h"

typedef struct {
  mixer *mixr;
  float left_phase;
  float right_phase;
} paData;

extern ENVSTREAM* ampstream;

mixer *new_mixer()
{
  mixer *mixr = NULL;
  mixr = calloc(1, sizeof(mixer));
  mixr->volume = 0.8;
  if (mixr == NULL) {
    printf("Nae mixer, fucked up!\n");
    return NULL;
  }
  return mixr;
}

void mixer_ps(mixer *mixr)
{
  printf(ANSI_COLOR_WHITE "::::: Mixing Desk (Volume: %f) :::::\n" ANSI_COLOR_RESET, mixr->volume);
  for ( int i = 0; i < mixr->soundgen_num; i++) {
    // printf("calling status on osc at %p\n", mixr->signals[i]);
    char ss[80];
    mixr->sound_generators[i]->status(mixr->sound_generators[i], ss);
    printf(ANSI_COLOR_YELLOW "SB [%d] - %s\n" ANSI_COLOR_RESET, i, ss); 
  }
  //for ( int i = 0; i < mixr->fmsig_num; i++) {
  //  char ss[80];
  //  fm_status(mixr->fmsignals[i], ss);
  //  printf(ANSI_COLOR_GREEN "FM [%d] - %s\n" ANSI_COLOR_RESET, i, ss); 
  //}
  //for ( int i = 0; i < mixr->sample_sig_num; i++) {
  //  char ss[120];
  //  sample_status(mixr->sample_signals[i], ss);
  //  printf(ANSI_COLOR_CYAN "Sample [%d] - %s\n" ANSI_COLOR_RESET, i, ss); 
  //}
}

void mixer_vol_change(mixer *mixr, float vol)
{
  printf("MIXER VOL CHANGE!\n");
  if (vol >= 0.0 && vol <= 1.0) {
    printf("PASSED THA CHALLEND WITH %F!\n", vol);
    mixr->volume = vol;
  }
}
void vol_change(mixer *mixr, int sig, float vol)
{
  if (sig > (mixr->sig_size - 1))
    return;
  mixr->signals[sig]->voladj(mixr->signals[sig], vol);
}

void freq_change(mixer *mixr, int sig, float freq)
{
  if (sig > (mixr->sig_size - 1))
    return;
  mixr->signals[sig]->freqadj(mixr->signals[sig], freq);
}

int add_osc(mixer *mixr, int freq, GTABLE *gt)
{
  //OSCIL **new_signals = NULL;
  ///* check if we need to allocate more space for OSCILs */
  //if (mixr->sig_size <= mixr->sig_num) {
  //  if (mixr->sig_size == 0) {
  //    mixr->sig_size = INITIAL_SIGNAL_SIZE;
  //  } else {
  //    mixr->sig_size *= 2;
  //  }

  //  new_signals = realloc(mixr->signals, mixr->sig_size *
  //                      sizeof(OSCIL*));
  //  if (new_signals == NULL) {
  //    printf("Unable to allocate more signalszzz");
  //    return mixr->sig_num;
  //  } else {
  //    mixr->signals = new_signals;
  //  }
  //}
  OSCIL *new_osc = new_oscil(freq, gt);
  if (new_osc == NULL) {
    printf("BARF!\n");
    return -1;
  }
  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    printf("MBARF!\n");
    return -1;
  }
  printf("HERE BEFORE CASTING!\n");
  m->sound_generator = (SOUNDGEN *) new_osc;
  m->freq = 1492;
  return add_sound_generator(mixr, m);
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
  printf("gota message here - freq = %d\n", sbm->freq);
  mixr->sound_generators[mixr->soundgen_num] = sbm->sound_generator;
  printf("AT END OF ADD_SOUND_GEN\n");
  return mixr->soundgen_num++;
}


int add_fm(mixer *mixr, int ffreq, int cfreq)
{
  //FM **new_fmsignals = NULL;
  ///* check if we need to allocate more space for FMszz */
  //if (mixr->fmsig_size <= mixr->fmsig_num) {
  //  if (mixr->fmsig_size == 0) {
  //    mixr->fmsig_size = INITIAL_SIGNAL_SIZE;
  //  } else {
  //    mixr->fmsig_size *= 2;
  //  }

  //  new_fmsignals = realloc(mixr->fmsignals, mixr->fmsig_size *
  //                      sizeof(FM*));
  //  if (new_fmsignals == NULL) {
  //    printf("Unable to allocate more FMsignalszzz");
  //    return mixr->fmsig_num;
  //  } else {
  //    mixr->fmsignals = new_fmsignals;
  //  }
  //}
  FM *nfm = new_fm(ffreq, cfreq);
  if (nfm == NULL) {
    printf("Barfed on FM creation\n");
    return -1;
  }
  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    printf("MBARF!\n");
    return -1;
  }
  m->sound_generator = (SOUNDGEN *) nfm;
  return add_sound_generator(mixr, m);

  //mixr->fmsignals[mixr->fmsig_num] = nfm;
  //return mixr->fmsig_num++;

  //OSCIL *new_osc = new_oscil(freq, gt);
  //if (new_osc == NULL) {
  //  printf("BARF!\n");
  //  return -1;
  //}
  //printf("HERE BEFORE CASTING!\n");
}

int add_sample(mixer *mixr, char *filename, char *pattern)
{

  // preliminary setup
  char cwd[1024];
  getcwd(cwd, 1024);
  char full_filename[strlen(filename) + strlen(cwd) + 7]; // 7 == '/wavs/' is 6 and 1 for '\0'
  strcpy(full_filename, cwd);
  strcat(full_filename, "/wavs/");
  strcat(full_filename, filename);

  SAMPLER *nsample = new_sampler(full_filename, pattern);
  if (nsample == NULL) {
    printf("Barfed on sample creation\n");
    return -1;
  }
  SBMSG *m = new_sbmsg();
  if (m == NULL) {
    // TODO: free ^ nsample - do the same for FM and OSC
    printf("MBARF!\n");
    return -1;
  }

  m->sound_generator = (SOUNDGEN *) nsample;
  return add_sound_generator(mixr, m);

  //if (nsample != NULL) {
  //  mixr->sample_signals[mixr->sample_sig_num] = nsample;
  //  return mixr->sample_sig_num++;
  //} else {
  //  printf("NEarly segfaulted there, pal.\n");
  //  return -1;
  //}
}
double gen_next(mixer *mixr)
{
  double output_val = 0.0;

  //if (mixr->sig_num > 0) {
  //  for (int i = 0; i < mixr->sig_num; i++) {
  //      output_val += mixr->signals[i]->tick(mixr->signals[i]);
  //  }
  //}

  //if (mixr->fmsig_num > 0) {
  //  for (int i = 0; i < mixr->fmsig_num; i++) {
  //    output_val += mixr->fmsignals[i]->gen_next(mixr->fmsignals[i]);
  //  }
  //}

  //if (mixr->sample_sig_num > 0) {
  //  for (int i = 0; i < mixr->sample_sig_num; i++) {
  //    output_val += mixr->sample_signals[i]->gen_next(mixr->sample_signals[i]);
  //  }
  //}

  if (mixr->soundgen_num > 0) {
    for (int i = 0; i < mixr->soundgen_num; i++) {
      output_val += mixr->sound_generators[i]->gennext(mixr->sound_generators[i]);
    }
  }
  // global envelope -- for the moment
  double mix_amp = envelope_stream_tick(ampstream);
  output_val *= mix_amp;
  return mixr->volume * (output_val / 1.53);
}

static int paCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
  paData *data = (paData*)userData;
  float *out = (float*)outputBuffer;
  unsigned int i;
  (void) inputBuffer;
  (void) timeInfo;
  (void) statusFlags;
  for (i = 0; i < framesPerBuffer; i++)
  {

    float val = gen_next(data->mixr);
    *out++ = val;
    *out++ = val;
  }
  return 0;
}

void *mixer_run(void *mixr_p)
{
  PaStream *stream;
  PaError err;
  paData data;
  data.mixr = (mixer*) mixr_p;

  err = Pa_OpenDefaultStream( &stream, 
                              0, // no input channels
                              2, // stereo output
                              paFloat32, // 32bit fp output
                              SAMPLE_RATE,
                              paFramesPerBufferUnspecified,
                              paCallback,
                              &data );

  if ( err != paNoError) {
    printf("Errrrr! couldn't open Portaudio default stream: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }

  err = Pa_StartStream( stream );
  if ( err != paNoError) {
    printf("Errrrr! couldn't start stream: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }

  // TODO: do i need this in a thread?
  // keep thread active
  while(1) {}

  return NULL;
}
