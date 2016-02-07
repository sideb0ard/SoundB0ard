#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <portaudio.h>

#include "envelope.h"
#include "fm.h"
#include "defjams.h"
#include "mixer.h"
#include "oscil.h"

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
  for ( int i = 0; i < mixr->sig_num; i++) {
    // printf("calling status on osc at %p\n", mixr->signals[i]);
    char ss[80];
    status(mixr->signals[i], ss);
    printf(ANSI_COLOR_YELLOW "SB [%d] - %s\n" ANSI_COLOR_RESET, i, ss); 
  }
  for ( int i = 0; i < mixr->fmsig_num; i++) {
    fm_status(mixr->fmsignals[i]);
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
  OSCIL **new_signals = NULL;
  /* check if we need to allocate more space for OSCILs */
  if (mixr->sig_size <= mixr->sig_num) {
    if (mixr->sig_size == 0) {
      mixr->sig_size = INITIAL_SIGNAL_SIZE;
    } else {
      mixr->sig_size *= 2;
    }

    new_signals = realloc(mixr->signals, mixr->sig_size *
                        sizeof(OSCIL*));
    if (new_signals == NULL) {
      printf("Unable to allocate more signalszzz");
      return mixr->sig_num;
    } else {
      mixr->signals = new_signals;
    }
  }
  OSCIL *new_osc = new_oscil(freq, gt);
  mixr->signals[mixr->sig_num] = new_osc;
  return mixr->sig_num++;
}

int add_fm(mixer *mixr, int ffreq, int cfreq)
{
  FM **new_fmsignals = NULL;
  /* check if we need to allocate more space for OSCILs */
  if (mixr->fmsig_size <= mixr->fmsig_num) {
    if (mixr->fmsig_size == 0) {
      mixr->fmsig_size = INITIAL_SIGNAL_SIZE;
    } else {
      mixr->fmsig_size *= 2;
    }

    new_fmsignals = realloc(mixr->fmsignals, mixr->fmsig_size *
                        sizeof(FM*));
    if (new_fmsignals == NULL) {
      printf("Unable to allocate more FMsignalszzz");
      return mixr->fmsig_num;
    } else {
      mixr->fmsignals = new_fmsignals;
    }
  }
  FM *nfm = new_fm(ffreq, cfreq);
  mixr->fmsignals[mixr->fmsig_num] = nfm;
  return mixr->fmsig_num++;
}

double gen_next(mixer *mixr)
{
  double output_val = 0.0;

  if (mixr->sig_num > 0) {
    for (int i = 0; i < mixr->sig_num; i++) {
      output_val += mixr->signals[i]->tick(mixr->signals[i]);
    }
  }

  if (mixr->fmsig_num > 0) {
    for (int i = 0; i < mixr->fmsig_num; i++) {
      output_val += mixr->fmsignals[i]->gen_next(mixr->fmsignals[i]);
    }
  }
  double amp = envelope_stream_tick(ampstream);
  output_val *= amp;

  if (output_val > 1.0)
    return 1.0;
  else
    return output_val;
  //return output_val;
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
