#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <portaudio.h>

#include "defjams.h"
#include "mixer.h"
#include "wave.h"

typedef struct {
  float left_phase;
  float right_phase;
} paData;

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
  for (i = 0; i < framesPerBuffer; i++)
  {
    *out++ = data->left_phase;
    *out++ = data->right_phase;
    data->left_phase += 0.01f;
    if (data->left_phase >= 0.1f) data->left_phase -= 2.0f;
    data->right_phase += 0.03f;
    if (data->right_phase >= 0.1f) data->right_phase -= 2.0f;
  }
  return 0;
}

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

void *mixer_run(void *args)
{
  PaStream *stream;
  PaError err;
  paData data;

  err = Pa_OpenDefaultStream( &stream, 
                              0, // no input channels
                              2, // stereo output
                              paFloat32, // 32bit fp output
                              SAMPLE_RATE,
                              paFramesPerBufferUnspecified,
                              paCallback,
                              &data );

  printf("OPENED STREAM FINE YO!\n");

  if ( err != paNoError) {
    printf("Errrrr! couldn't open Portaudio default stream: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }

  //err = Pa_StartStream( stream );
  //if ( err != paNoError) {
  //  printf("Errrrr! couldn't start stream: %s\n", Pa_GetErrorText(err));
  //  exit(-1);
  //}
  //while(1)
  //{
  //  printf("Mixingg away...\n");
  //  sleep(2);
  //}
  return NULL;
}

void mixer_ps(mixer *mixr)
{
  printf("::::: Mixing Desk :::::\n");
  for ( int i = 0; i < mixr->num_sig; i++) {
    printf("calling status on osc at %p\n", mixr->signals[i]);
    char ss[80];
    status(mixr->signals[i], ss);
    printf("SB [%d] - %s\n", i, ss); 
  }
}

void add_osc(mixer *mixr, uint32_t freq)
{
  OSCIL **new_signals = NULL;
  /* check if we need to allocate more space for OSCILs */
  printf("BZZT! signals at mem: %p\n", mixr->signals);
  if (mixr->sig_size <= mixr->num_sig) {
    if (mixr->sig_size == 0) {
      mixr->sig_size = INITIAL_SIGNAL_SIZE;
      printf("YEY! sigsize is now inital size: %d\n", mixr->sig_size);
    } else {
      mixr->sig_size *= 2;
      printf("OYEY! sigsize now doubled: %d\n", mixr->sig_size);
    }

    new_signals = realloc(mixr->signals, mixr->sig_size *
                        sizeof(OSCIL*));
    if (new_signals == NULL) {
      printf("Unable to allocate more paths");
      return;
    } else {
      mixr->signals = new_signals;
      printf("BOOM! realloced singals: %p\n", mixr->signals);
    }
  }
  OSCIL *new_osc = new_oscil(freq);
  mixr->signals[mixr->num_sig] = new_osc;
  mixr->num_sig++;
}

