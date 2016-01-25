#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <portaudio.h>

#include "defjams.h"
#include "mixer.h"
#include "oscil.h"

typedef struct {
  mixer *mixr;
  float left_phase;
  float right_phase;
} paData;

mixer *new_mixer()
{
  mixer *mixr = NULL;
  mixr = calloc(1, sizeof(mixer));
  if (mixr == NULL) {
    printf("Nae mixer, fucked up!\n");
    return NULL;
  }
  return mixr;
}

void mixer_ps(mixer *mixr)
{
  printf(ANSI_COLOR_WHITE "::::: Mixing Desk :::::\n" ANSI_COLOR_RESET);
  for ( int i = 0; i < mixr->num_sig; i++) {
    // printf("calling status on osc at %p\n", mixr->signals[i]);
    char ss[80];
    status(mixr->signals[i], ss);
    printf(ANSI_COLOR_YELLOW "SB [%d] - %s\n" ANSI_COLOR_RESET, i, ss); 
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

void add_osc(mixer *mixr, int freq, GTABLE *gt)
{
  OSCIL **new_signals = NULL;
  /* check if we need to allocate more space for OSCILs */
  //printf("BZZT! signals at mem: %p\n", mixr->signals);
  if (mixr->sig_size <= mixr->num_sig) {
    if (mixr->sig_size == 0) {
      mixr->sig_size = INITIAL_SIGNAL_SIZE;
      //printf("YEY! sigsize is now inital size: %d\n", mixr->sig_size);
    } else {
      mixr->sig_size *= 2;
      //printf("OYEY! sigsize now doubled: %d\n", mixr->sig_size);
    }

    new_signals = realloc(mixr->signals, mixr->sig_size *
                        sizeof(OSCIL*));
    if (new_signals == NULL) {
      printf("Unable to allocate more signalszzz");
      return;
    } else {
      mixr->signals = new_signals;
      //printf("BOOM! realloced singals: %p\n", mixr->signals);
    }
  }
  OSCIL *new_osc = new_oscil(freq, gt);
  mixr->signals[mixr->num_sig] = new_osc;
  mixr->num_sig++;
}


double gen_next(mixer *mixr)
{
  double output_val = 0.0;
  if (mixr->num_sig > 0) {
    for (int i = 0; i < mixr->num_sig; i++) {
      //output_val += sinetick(mixr->signals[i]);
      output_val += mixr->signals[i]->tick(mixr->signals[i]);
      //printf("[%d] - %f\n", i, output_val);
    }
  }
  return output_val;
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

  // keep thread active
  while(1) {}

  return NULL;
}
