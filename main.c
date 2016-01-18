#include <portaudio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "cmdinterpreter.h"
#include "defjams.h"
#include "mixer.h"

mixer *mixr;

int main(int argc, char **argv)
{
  // PA start me up!
  PaError err;
  err = Pa_Initialize();
  if ( err != paNoError) {
    printf("Errrrr! couldn't initialize Portaudio: %s\n", Pa_GetErrorText(err));
    exit(-1);
  } 


  // run da mixer
  mixr = new_mixer();
  pthread_t mixrrun_th;
  if ( pthread_create (&mixrrun_th, NULL, mixer_run, (void*) mixr)) {
    fprintf(stderr, "Error running mixer_run thread\n");
    return 1;
  }
  pthread_detach(mixrrun_th);

  // interactive loop
  cmd_loopy();

  // all done, time to go home
  err = Pa_Terminate();
  if( err != paNoError )
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );

  return 0;
}
