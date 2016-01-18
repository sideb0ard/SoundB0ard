#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>

#include "audioutils.h"

void pa_setup(void)
{
  // PA start me up!
  PaError err;
  err = Pa_Initialize();
  if ( err != paNoError) {
    printf("Errrrr! couldn't initialize Portaudio: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }
}

void pa_teardown(void)
{
  //  time to go home!
  PaError err;
  err = Pa_Terminate();
  if ( err != paNoError) {
    printf("Errrrr while terminating Portaudio: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }
}
