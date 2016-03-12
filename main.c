#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "audioutils.h"
#include "envelope.h"
#include "bpmrrr.h"
#include "cmdloop.h"
#include "defjams.h"
#include "mixer.h"

mixer *mixr;
bpmrrr *b;
GTABLE *sine_table;
GTABLE *tri_table;
GTABLE *square_table;
GTABLE *saw_up_table;
GTABLE *saw_down_table;

ENVSTREAM* ampstream = NULL;

static int paCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
  paData *data = (paData*)userData;
  float *out = (float*)outputBuffer;
  (void) inputBuffer;
  (void) timeInfo;
  (void) statusFlags;

  //int delay_p = data->delay_p;
  //float *delay = data->delay;
  int delay_p = mixr->effects[0]->buf_p;
  float *delay = mixr->effects[0]->buffer;

  for (unsigned long i = 0; i < framesPerBuffer; i++) {
    float val = 0;
    if (data->mixr->delay_on)
      val = delay[delay_p];
      //val = mixr->effects[0]->buffer[mixr->effects[0]->buf_p];
    else
      val = gen_next(data->mixr);
    delay[delay_p++] = gen_next(data->mixr) + val*0.5;
    if (delay_p >= SAMPLE_RATE/8) delay_p = 0; 
    //mixr->effects[0]->buffer[mixr->effects[0]->buf_p++] = gen_next(data->mixr) + val*0.5;
    //if  (mixr->effects[0]->buf_p <= SAMPLE_RATE/8) mixr->effects[0]->buf_p = 0;
    //val = mixr->effects[0]->buffer[mixr->effects[0]->buf_p];
    //val = mixr->effects[0]->buffer[mixr->effects[0]->buf_p];
    *out++ = val;
    *out++ = val;
  }
  //data->delay_p = delay_p;
  mixr->effects[0]->buf_p = delay_p;
  return 0;
}

int main()
{

  // lookup table for wavs
  sine_table = new_sine_table();
  tri_table = new_tri_table();
  square_table = new_square_table();
  saw_up_table = new_saw_table(1);
  saw_down_table = new_saw_table(0);

  ampstream = new_envelope_stream();

  // run da BPM counterrr...
  b = new_bpmrrr();
  pthread_t bpmrun_th;
  if ( pthread_create (&bpmrun_th, NULL, bpm_run, (void*) b)) {
    fprintf(stderr, "Error running BPM_run thread\n");
    return 1;
  }
  pthread_detach(bpmrun_th);


  // PortAudio start me up!
  pa_setup();

  PaStream *stream;
  PaError err;
  paData *data = calloc(1, sizeof(paData));
  mixr = new_mixer();
  add_effect(mixr); // tmp
  data->mixr = mixr;

  err = Pa_OpenDefaultStream( &stream, 
                              0, // no input channels
                              2, // stereo output
                              paFloat32, // 32bit fp output
                              SAMPLE_RATE,
                              paFramesPerBufferUnspecified,
                              paCallback,
                              data );

  if ( err != paNoError) {
    printf("Errrrr! couldn't open Portaudio default stream: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }

  err = Pa_StartStream( stream );
  if ( err != paNoError) {
    printf("Errrrr! couldn't start stream: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }


  loopy();

  // all done, time to go home
  pa_teardown();

  return 0;
}
