#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

#include <iostream>

#include "ableton_link_wrapper.h"
#include "audioutils.h"
#include "cmdloop.h"
#include "defjams.h"
#include "midimaaan.h"
#include "mixer.h"

#include <interpreter/evaluator.hpp>
#include <interpreter/object.hpp>
#include <tsqueue.hpp>

extern mixer *mixr;

Tsqueue<mixer_timing_info> g_event_queue;

extern const wchar_t *sparkchars;
extern const char *key_names[NUM_KEYS];
extern const char *prompt;
extern char *chord_type_names[NUM_CHORD_TYPES];

std::mutex g_stdout_mutex;

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

void *process_worker_thread(void *)
{
    while (auto const timing_info = g_event_queue.pop())
    {
        if (timing_info)
        {
            if (mixr->proc_initialized_)
            {
                for (auto p : mixr->processes_)
                {
                    if (p->active_)
                        p->EventNotify(*timing_info);
                }
            }
        }
    }
    return nullptr;
}

int main()
{

    srand(time(NULL));
    signal(SIGINT, SIG_IGN);

    double output_latency = pa_setup();
    mixr = new_mixer(output_latency);

    PaStream *output_stream;
    PaError err;

    err = Pa_OpenDefaultStream(&output_stream,
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

    err = Pa_StartStream(output_stream);
    if (err != paNoError)
    {
        printf("Errrrr! couldn't start stream: %s\n", Pa_GetErrorText(err));
        exit(-1);
    }

    // Command Loop
    pthread_t input_th;
    if (pthread_create(&input_th, NULL, loopy, NULL))
    {
        fprintf(stderr, "Errrr, wit tha Loopy..\n");
    }

    // Worker Loop
    pthread_t worker_th;
    if (pthread_create(&worker_th, NULL, process_worker_thread, NULL))
    {
        fprintf(stderr, "Errrr, wit tha Wurker..\n");
    }

    pthread_join(input_th, NULL);
    g_event_queue.close();
    pthread_join(worker_th, NULL);

    // all done, time to go home
    pa_teardown();
    if (mixr->have_midi_controller)
    {
        Pm_Close(mixr->midi_stream);
        Pm_Terminate();
    }

    return 0;
}
