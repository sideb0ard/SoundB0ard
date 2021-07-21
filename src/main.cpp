#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>

#include <iostream>

#include <lo/lo.h>
#include <lo/lo_cpp.h>

#include "PerlinNoise.hpp"

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
#include <process.hpp>
#include <tsqueue.hpp>

extern Mixer *mixr;

Tsqueue<audio_action_queue_item> audio_queue;
Tsqueue<int> audio_reply_queue; // for reply from adding SoundGenerator
Tsqueue<std::string> eval_command_queue;
Tsqueue<std::string> repl_queue;
Tsqueue<event_queue_item> process_event_queue;

siv::PerlinNoise perlinGenerator; // only for use by eval thread

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
    Mixer *mixr = (Mixer *)user_data;

    int ret = mixr->GenNext(out, frames_per_buffer);

    return ret;
}

void Eval(char *line, std::shared_ptr<object::Environment> env)
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

void *eval_queue()
{
    while (auto cmd = eval_command_queue.pop())
    {
        if (cmd)
        {
            Eval(cmd->data(), global_env);
        }
    }
    return nullptr;
}

void *process_worker_thread()
{
    while (auto const event = process_event_queue.pop())
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
                    ProcessConfig config = {

                        .name = event->process_name,
                        .process_type = event->process_type,
                        .timer_type = event->timer_type,
                        .loop_len = event->loop_len,
                        .command = event->command,
                        .target_type = event->target_type,
                        .targets = event->targets,
                        .pattern_expression = event->pattern_expression,
                        .pattern = "",
                        .funcz = event->funcz,
                        .tinfo = event->timing_info};

                    mixr->processes_[event->target_process_id]->EnqueueUpdate(
                        config);
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

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, lo_message data, void *user_data)
{
    int i;

    printf("generic handler; path: <%s>\n", path);
    for (i = 0; i < argc; i++)
    {
        printf("arg %d '%c' ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }
    printf("\n");
    fflush(stdout);

    return 1;
}

int main()
{

    srand(time(NULL));
    signal(SIGINT, SIG_IGN);

    double output_latency = pa_setup();
    mixr = new Mixer(output_latency);

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

    // REPL
    std::thread repl_thread(loopy);

    // Processes
    std::thread worker_thread(process_worker_thread);

    // Eval loop
    std::thread eval_thread(eval_queue);

    // OSC Server
    lo::ServerThread st(9000);
    if (!st.is_valid())
    {
        std::cout << "Already Running?\n\n" << std::endl;
        return 1;
    }

    /* Set some lambdas to be called when the thread starts and
     * ends. Here we demonstrate capturing a reference to the server
     * thread. */
    // st.set_callbacks([&st]() { printf("Thread init: %p.\n", &st); },
    //                 []() { printf("Thread cleanup.\n"); });

    // std::cout << "URL: " << st.url() << std::endl;

    std::atomic<int> received(0);

    /*
     * Add a method handler for "/example,i" using a C++11 lambda to
     * keep it succinct.  We capture a reference to the `received'
     * count and modify it atomatically.
     *
     * You can also pass in a normal function, or a callable function
     * object.
     *
     * Note: If the lambda doesn't specify a return value, the default
     *       is `return 0', meaning "this message has been handled,
     *       don't continue calling the method chain."  If this is not
     *       the desired behaviour, add `return 1' to your method
     *       handlers.
     */
    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);
    st.add_method("noteOn", "si",
                  [&received](lo_arg **argv, int)
                  {
                      std::cout << "noteOn (" << (++received)
                                << "): " << &argv[0]->s << " " << argv[1]->i
                                << std::endl;
                      std::stringstream ss;
                      ss << "noteOn(" << &argv[0]->s << ", " << argv[1]->i
                         << ");";
                      std::cout << "CMD is " << ss.str();

                      eval_command_queue.push(ss.str());
                  });

    /*
     * Start the server.
     */
    st.start();

    //////////////// shutdown

    repl_thread.join();

    process_event_queue.close();
    worker_thread.join();

    eval_command_queue.close();
    eval_thread.join();

    // all done, time to go home
    pa_teardown();
    if (mixr->have_midi_controller)
    {
        Pm_Close(mixr->midi_stream);
        Pm_Terminate();
    }

    return 0;
}
