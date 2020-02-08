#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include <readline/history.h>
#include <readline/readline.h>

#include <iostream>

#include <algo_cmds.h>
#include <cmdloop.h>
#include <looper_cmds.h>
#include <midi_cmds.h>
#include <mixer.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <obliquestrategies.h>
#include <pattern_generator_cmds.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>
#include <tsqueue.hpp>
#include <utils.h>
#include <value_generator_cmds.h>

extern mixer *mixr;
extern Tsqueue<std::string> g_command_queue;

extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
#define READLINE_SAFE_RESET "\001\x1b[0m\002"
#define MAXLINE 128

char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;
static char last_line[MAXLINE] = {};
static bool active{true};

void *loopy()
{
    print_logo();
    read_history(NULL);
    setlocale(LC_ALL, "");

    char *line;
    while ((line = readline(prompt)) != NULL && active)
    {
        if (line && *line)
        {
            if (strncmp(last_line, line, MAXLINE) != 0)
            {
                add_history(line);
                strncpy(last_line, line, MAXLINE);
            }
            g_command_queue.push(line);
            free(line);
        }
    }
    exxit();

    return NULL;
}

int exxit()
{
    printf(COOL_COLOR_PINK
           "\nBeat it, ya val jerk!\n" ANSI_COLOR_RESET); // Thrashin' reference
    write_history(NULL);

    pa_teardown();

    active = false;

    return 0;
}

int generic_osc_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    (void)argc;
    (void)data;
    (void)user_data;
    int i;

    printf("path: <%s>\n", path);
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

int trigger_osc_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;
    // printf("Target %d\n", argv[0]->i);
    int target_sg = argv[0]->i;
    fflush(stdout);
    midi_event _event = new_midi_event(MIDI_ON, 32, 128);
    _event.source = EXTERNAL_OSC;
    if (mixer_is_valid_soundgen_num(mixr, target_sg))
    {
        std::shared_ptr<SoundGenerator> sg = mixr->SoundGenerators[target_sg];
        sg->parseMidiEvent(_event, mixr->timing_info);
    }

    return 0;
}

int osc_note_on_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    (void)path;
    (void)types;
    (void)argc;
    (void)data;
    (void)user_data;

    int target_sg = argv[0]->i;
    int midi_note = argv[1]->i;
    int octave = argv[2]->i;
    int velocity = argv[3]->i;

    midi_event _event =
        new_midi_event(MIDI_ON, (octave * 12) + midi_note, velocity);
    _event.source = EXTERNAL_OSC;
    if (mixer_is_valid_soundgen_num(mixr, target_sg))
    {
        std::shared_ptr<SoundGenerator> sg = mixr->SoundGenerators[target_sg];
        sg->parseMidiEvent(_event, mixr->timing_info);
    }
    return 0;
}
