#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

#include "audioutils.h"
#include "bpmrrr.h"
#include "cmdloop.h"
#include "defjams.h"
#include "envelope.h"
#include "midimaaan.h"
#include "mixer.h"

mixer *mixr;
bpmrrr *b;

// use broadcast to wake up threads when tick changes in bpm
pthread_cond_t bpm_cond;
pthread_mutex_t bpm_lock;

static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags, void *userData)
{
    paData *data = (paData *)userData;
    float *out = (float *)outputBuffer;
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        float val = 0;
        val = gen_next(data->mixr);
        *out++ = val;
        *out++ = val;
    }
    return 0;
}

int main()
{

    pthread_mutex_init(&bpm_lock, NULL);
    pthread_cond_init(&bpm_cond, NULL);

    // run da BPM counterrr...
    b = new_bpmrrr();
    pthread_t bpmrun_th;
    if (pthread_create(&bpmrun_th, NULL, bpm_run, (void *)b)) {
        fprintf(stderr, "Error running BPM_run thread\n");
        return 1;
    }
    pthread_detach(bpmrun_th);

    //// run the MIDI event looprrr...
    pthread_t midi_th;
    if (pthread_create(&midi_th, NULL, midiman, NULL)) {
        fprintf(stderr, "Errrr, wit tha midi..\n");
        return -1;
    }
    pthread_detach(midi_th);


    // PortAudio start me up!
    pa_setup();

    PaStream *stream;
    PaError err;
    paData *data = calloc(1, sizeof(paData));
    mixr = new_mixer();
    data->mixr = mixr;

    err = Pa_OpenDefaultStream(&stream,
                               0,         // no input channels
                               2,         // stereo output
                               paFloat32, // 32bit fp output
                               SAMPLE_RATE, paFramesPerBufferUnspecified,
                               paCallback, data);

    if (err != paNoError) {
        printf("Errrrr! couldn't open Portaudio default stream: %s\n",
               Pa_GetErrorText(err));
        exit(-1);
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("Errrrr! couldn't start stream: %s\n", Pa_GetErrorText(err));
        exit(-1);
    }

    loopy();

    // all done, time to go home
    pa_teardown();

    return 0;
}
