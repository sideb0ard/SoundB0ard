#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>

#include <iostream>

#include "ableton_link_wrapper.h"
#include "audioutils.h"
#include "cmdloop.h"
#include "defjams.h"
#include "midimaaan.h"
#include "mixer.h"

#include <interpreter/evaluator.hpp>
#include <interpreter/lexer.hpp>
#include <interpreter/object.hpp>
#include <interpreter/parser.hpp>
#include <interpreter/token.hpp>

#include <audio_action_queue.h>
#include <event_queue.h>
#include <tsqueue.hpp>

extern mixer *mixr;

Tsqueue<audio_action_queue_item> g_audio_action_queue;
Tsqueue<std::string> g_command_queue;
Tsqueue<std::string> g_reply_queue;
Tsqueue<event_queue_item> g_event_queue;

auto global_env = std::make_shared<object::Environment>();

extern const wchar_t *sparkchars;
extern const char *key_names[NUM_KEYS];
extern const char *prompt;
extern char *chord_type_names[NUM_CHORD_TYPES];

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

void Interpret(char *line, std::shared_ptr<object::Environment> env)
{
    auto lex = std::make_shared<lexer::Lexer>();
    lex->ReadInput(line);
    auto parsley = std::make_unique<parser::Parser>(lex);

    std::shared_ptr<ast::Program> program = parsley->ParseProgram();

    auto evaluated = evaluator::Eval(program, env);
    if (evaluated)
    {
        auto result = evaluated->Inspect();
        if (result.compare("null") != 0)
        {
            std::cout << result << std::endl;
        }
    }
}

void *command_queue_thread()
{
    while (auto cmd = g_command_queue.pop())
    {
        if (cmd)
        {
            Interpret(cmd->data(), global_env);
        }
    }
    return nullptr;
}

void *process_worker_thread()
{
    while (auto const event = g_event_queue.pop())
    {
        if (event)
        {
            if (event->type == Event::TIMING_EVENT)
            {
                auto timing_info = event->timing_info;

                if (mixr->proc_initialized_)
                {
                    for (auto p : mixr->processes_)
                    {
                        if (p->active_)
                            p->EventNotify(timing_info);
                    }
                }
            }
            else if (event->type == Event::PROCESS_UPDATE_EVENT)
            {
                if (event->target_process_id >= 0 &&
                    event->target_process_id < MAX_NUM_PROC)
                {
                    mixr->processes_[event->target_process_id]->Update(
                        event->process_type, event->timer_type, event->loop_len,
                        event->command, event->target_type, event->targets,
                        event->pattern, event->funcz);
                }
                else
                {
                    std::cerr << "WAH! INVALID process id: "
                              << event->target_process_id << std::endl;
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

    // Cmd loop
    std::thread input_thread(loopy);

    // Processes
    std::thread worker_thread(process_worker_thread);

    // Interpreter
    std::thread command_thread(command_queue_thread);

    //////////////// shutdown

    input_thread.join();

    g_event_queue.close();
    worker_thread.join();

    g_command_queue.close();
    command_thread.join();

    // all done, time to go home
    pa_teardown();
    if (mixr->have_midi_controller)
    {
        Pm_Close(mixr->midi_stream);
        Pm_Terminate();
    }

    return 0;
}
