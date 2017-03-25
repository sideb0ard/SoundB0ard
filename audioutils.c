#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>

#include "audioutils.h"

double pa_setup(void)
{
    // PA start me up!
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        printf("Errrrr! couldn't initialize Portaudio: %s\n",
               Pa_GetErrorText(err));
        exit(-1);
    }
    PaStreamParameters params;
    params.device = Pa_GetDefaultOutputDevice();
    if (params.device == paNoDevice)
        printf("BARF!\n");
    double suggested_latency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
    printf("SUGGESTED LATENCY: %f\n", suggested_latency);
    return suggested_latency;
}

void pa_teardown(void)
{
    //  time to go home!
    PaError err;
    err = Pa_Terminate();
    if (err != paNoError) {
        printf("Errrrr while terminating Portaudio: %s\n",
               Pa_GetErrorText(err));
        exit(-1);
    }
    exit(0);
}
