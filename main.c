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

const char *key_names[] = {
    "C_MAJOR",      "G_MAJOR",      "D_MAJOR",       "A_MAJOR",
    "E_MAJOR",      "B_MAJOR",      "F_SHARP_MAJOR", "D_FLAT_MAJOR",
    "A_FLAT_MAJOR", "E_FLAT_MAJOR", "B_FLAT_MAJOR",  "F_MAJOR",
};

const int key_midi_mapping[] = {24, 31, 26, 33, 28, 35, 30, 25, 32, 27, 34, 29};

// typedef unsigned int compat_key_list[6];
const compat_key_list compat_keys[] = {
    {C_MAJOR, D_FLAT_MAJOR, E_FLAT_MAJOR, F_MAJOR, G_MAJOR, A_FLAT_MAJOR},
    {G_MAJOR, A_FLAT_MAJOR, B_FLAT_MAJOR, C_MAJOR, D_MAJOR, E_FLAT_MAJOR},
    {D_MAJOR, E_FLAT_MAJOR, F_SHARP_MAJOR, G_MAJOR, A_MAJOR, B_FLAT_MAJOR},
    {A_MAJOR, B_FLAT_MAJOR, D_FLAT_MAJOR, D_MAJOR, E_MAJOR, F_SHARP_MAJOR},
    {E_MAJOR, F_SHARP_MAJOR, A_FLAT_MAJOR, A_MAJOR, B_MAJOR, D_FLAT_MAJOR},
    {B_MAJOR, D_FLAT_MAJOR, E_FLAT_MAJOR, E_MAJOR, F_SHARP_MAJOR, A_FLAT_MAJOR},
    {F_SHARP_MAJOR, A_FLAT_MAJOR, B_FLAT_MAJOR, B_MAJOR, D_FLAT_MAJOR,
     E_FLAT_MAJOR},
    {D_FLAT_MAJOR, E_FLAT_MAJOR, E_MAJOR, F_SHARP_MAJOR, A_FLAT_MAJOR,
     B_FLAT_MAJOR},
    {A_FLAT_MAJOR, B_FLAT_MAJOR, B_MAJOR, D_FLAT_MAJOR, E_FLAT_MAJOR, E_MAJOR},
    {E_FLAT_MAJOR, E_MAJOR, F_SHARP_MAJOR, A_FLAT_MAJOR, B_FLAT_MAJOR, B_MAJOR},
    {B_FLAT_MAJOR, B_MAJOR, D_FLAT_MAJOR, E_FLAT_MAJOR, F_MAJOR, F_SHARP_MAJOR},
    {F_MAJOR, F_SHARP_MAJOR, A_FLAT_MAJOR, B_FLAT_MAJOR, C_MAJOR,
     D_FLAT_MAJOR}};

static int paCallback(const void *input_buffer, void *output_buffer,
                      unsigned long frames_per_buffer,
                      const PaStreamCallbackTimeInfo *time_info,
                      PaStreamCallbackFlags status_flags, void *user_data)
{
    mixer *mixr = (mixer *)user_data;
    float *out = (float *)output_buffer;
    (void)input_buffer;
    (void)time_info;
    (void)status_flags;

    int ret = mixer_gennext(mixr, out, frames_per_buffer);

    return ret;
}

int main()
{

    srand(time(NULL));

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

    loopy();

    // all done, time to go home
    pa_teardown();

    return 0;
}
