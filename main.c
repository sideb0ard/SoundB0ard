#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

#include "ableton_link_wrapper.h"
#include "audioutils.h"
#include "cmdloop.h"
#include "defjams.h"
#include "envelope.h"
#include "midimaaan.h"
#include "mixer.h"

mixer *mixr;
const wchar_t *sparkchars = L"\u2581\u2582\u2583\u2585\u2586\u2587";

const char *key_names[] = {"C", "C_SHARP", "D", "D_SHARP", "E", "F", "F_SHARP",
                           "G", "G_SHARP", "A", "A_SHARP", "B"};

const char *chord_type_names[] = {"MAJOR", "MINOR", "DIMINISHED"};

static int paCallback(const void *input_buffer, void *output_buffer,
                      unsigned long frames_per_buffer,
                      const PaStreamCallbackTimeInfo *time_info,
                      PaStreamCallbackFlags status_flags, void *user_data)
{
    (void)input_buffer;
    (void)time_info;
    (void)status_flags;

    float *out = (float *)output_buffer;
    mixer *mixr = (mixer *)user_data;

    int ret = mixer_gennext(mixr, out, frames_per_buffer);

    return ret;
}

int main()
{

    srand(time(NULL));

    double output_latency = pa_setup();
    mixr = new_mixer(output_latency);

    PaStream *stream;
    PaError err;

    err = Pa_OpenDefaultStream(&stream,
                               0,         // no input channels
                               2,         // stereo output
                               paFloat32, // 32bit fp output
                               SAMPLE_RATE, paFramesPerBufferUnspecified,
                               paCallback, mixr);

    if (err != paNoError)
    {
        printf("Errrrr! couldn't open Portaudio default stream: %s\n",
               Pa_GetErrorText(err));
        exit(-1);
    }

    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        printf("Errrrr! couldn't start stream: %s\n", Pa_GetErrorText(err));
        exit(-1);
    }

    pthread_t input_th;
    if (pthread_create(&input_th, NULL, loopy, NULL))
    {
        fprintf(stderr, "Errrr, wit tha midi..\n");
    }
    pthread_join(input_th, NULL);

    // all done, time to go home
    pa_teardown();
    if (mixr->have_midi_controller)
    {
        Pm_Close(mixr->midi_stream);
        Pm_Terminate();
    }

    return 0;
}
