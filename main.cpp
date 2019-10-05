#include <pthread.h>
#include <signal.h>
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

#include <interpreter/evaluator.hpp>
#include <interpreter/object.hpp>
#include <tsqueue.hpp>

mixer *mixr;

using Wrapper =
    std::pair<std::shared_ptr<ast::Node>, std::shared_ptr<object::Environment>>;
Tsqueue<Wrapper> g_queue;

const wchar_t *sparkchars = L"\u2581\u2582\u2583\u2585\u2586\u2587";

const char *key_names[] = {"C", "C_SHARP", "D", "D_SHARP", "E", "F", "F_SHARP",
                           "G", "G_SHARP", "A", "A_SHARP", "B"};

const char *chord_type_names[] = {"MAJOR", "MINOR", "DIMINISHED"};

extern char const *prompt;

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

void *Evaluator(void *arg)
{
    while (auto const &Wrapper = g_queue.pop())
    {
        if (Wrapper)
        {
            auto &[node, env] = *Wrapper;
            auto evaluated = evaluator::Eval(node, env);
            if (evaluated)
            {
                auto result = evaluated->Inspect();
                if (result.compare("null") != 0)
                {
                    std::cout << result << std::endl;
                    // std::cout << prompt;
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

    pthread_t input_th;
    if (pthread_create(&input_th, NULL, loopy, NULL))
    {
        fprintf(stderr, "Errrr, wit tha Loopy..\n");
    }

    pthread_t eval_th;
    if (pthread_create(&eval_th, NULL, Evaluator, NULL))
    {
        fprintf(stderr, "Errrr, wit tha Evaluator thread!..\n");
    }

    pthread_join(input_th, NULL);
    pthread_join(eval_th, NULL);

    // all done, time to go home
    pa_teardown();
    if (mixr->have_midi_controller)
    {
        Pm_Close(mixr->midi_stream);
        Pm_Terminate();
    }

    return 0;
}
