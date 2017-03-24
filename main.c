#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

#include "audioutils.h"
#include "cmdloop.h"
#include "defjams.h"
#include "envelope.h"
#include "midimaaan.h"
#include "mixer.h"

mixer *mixr;
const wchar_t *sparkchars = L"\u2581\u2582\u2583\u2585\u2586\u2587";

// use broadcast to wake up threads when midi tick changes
pthread_cond_t midi_tick_cond;
pthread_mutex_t midi_tick_lock;

static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags, void *userData)
{
    mixer *mixr = (mixer *)userData;
    float *out = (float *)outputBuffer;
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    // Ableton Sync stuff //////////////////////////////////////////////
    link_update_from_main_callback(mixr->m_ableton_link);
    /////////////////////////////////////////////////////////////////////

    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        float val = 0;
        val = mixer_gennext(mixr, i);
        *out++ = val;
        *out++ = val;
    }


    return 0;
}

int main()
{

    pthread_mutex_init(&midi_tick_lock, NULL);
    pthread_cond_init(&midi_tick_cond, NULL);

    srand(time(NULL));

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
    mixr = new_mixer();

    err = Pa_OpenDefaultStream(&stream,
                               0,         // no input channels
                               2,         // stereo output
                               paFloat32, // 32bit fp output
                               SAMPLE_RATE, paFramesPerBufferUnspecified,
                               paCallback, mixr);

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
