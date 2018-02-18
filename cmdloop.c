#include <locale.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wchar.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "algorithm.h"
#include "basicfilterpass.h"
#include "beatrepeat.h"
#include "bitcrush.h"
#include "cmdloop.h"
#include "defjams.h"
#include "digisynth.h"
#include "distortion.h"
#include "dxsynth.h"
#include "dynamics_processor.h"
#include "envelope.h"
#include "envelope_follower.h"
#include "euclidean.h"
#include "help.h"
#include "keys.h"
#include "looper.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "modfilter.h"
#include "modular_delay.h"
#include "obliquestrategies.h"
#include "oscillator.h"
#include "pattern_parser.h"
#include "pattern_transformers.h"
#include "reverb.h"
#include "sample_sequencer.h"
#include "sequencer_utils.h"
#include "sparkline.h"
#include "synthbase.h"
#include "synthdrum_sequencer.h"
#include "table.h"
#include "utils.h"
#include "waveshaper.h"

#include <fx_cmds.h>
#include <looper_cmds.h>
#include <mixer_cmds.h>
#include <new_item_cmds.h>
#include <pattern_cmds.h>
#include <stepper_cmds.h>
#include <synth_cmds.h>

extern mixer *mixr;
extern char *key_names[NUM_KEYS];

extern wtable *wave_tables[5];

//#define READLINE_SAFE_MAGENTA "\001\x1b[35m\002"
//#define READLINE_SAFE_RESET "\001\x1b[0m\002"
// char const *prompt = READLINE_SAFE_MAGENTA "SB#> " READLINE_SAFE_RESET;
// Turns out OSX uses NetBSD editline to implement readline and it has a
// bug that means color prompt is broke -
// https://stackoverflow.com/questions/31329952/colorized-readline-prompt-breaks-control-a
// TODO(find a fix)
char const *prompt = "SB#> ";

static void print_logo()
{
    printf(
        ANSI_COLOR_WHITE
        "Welcome to...\n" COOL_COLOR_GREEN
        " _____                       _ _     _____               _\n"
        "/  ___|                     | | |   |  _  |             | |\n"
        "\\ `--.  ___  _   _ _ __   __| | |__ | |/\" | __ _ _ __ __| |\n"
        " `--. \\/ _ \\| | | | \"_ \\ / _` | \"_ \\|  /| |/ _` | \"__/ _` |\n"
        "/\\__/ / (_) | |_| | | | | (_| | |_) \\ |_/ / (_| | | | (_| |\n"
        "\\____/ \\___/ \\__,_|_| |_|\\__,_|_.__/ \\___/ \\__,_|_|  "
        "\\__,_|\n\n" ANSI_COLOR_RESET);
}

void *loopy(void *arg)
{
    read_history(NULL);

    setlocale(LC_ALL, "");

    print_logo();

    char *line;
    while ((line = readline(prompt)) != NULL)
    {
        // if (line[0] != 0)
        if (line && *line)
        {
            add_history(line);
            interpret(line);
            free(line);
        }
    }
    write_history(NULL);
    exxit();

    return NULL;
}

void interpret(char *line)
{
    char wurds[NUM_WURDS][SIZE_OF_WURD];

    char *cmd, *last_s;
    char const *sep = ";";
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s))
    {

        char tmp[1024];
        strncpy((char *)tmp, cmd, 127);

        int num_wurds = parse_wurds_from_cmd(wurds, tmp);

        //////////////////////////////////////////////////////////////////////

        if (strncmp("help", wurds[0], 4) == 0)
            oblique_strategy();

        else if (strncmp("quit", wurds[0], 4) == 0 ||
                 strncmp("exit", wurds[0], 4) == 0)
            exxit();

        else if (parse_fx_cmd(num_wurds, wurds))
            continue;

        else if (parse_looper_cmd(num_wurds, wurds))
            continue;

        else if (parse_mixer_cmd(num_wurds, wurds))
            continue;

        else if (parse_new_item_cmd(num_wurds, wurds))
            continue;

        else if (parse_pattern_cmd(num_wurds, wurds))
            continue;

        else if (parse_synth_cmd(num_wurds, wurds))
            continue;

        else if (parse_stepper_cmd(num_wurds, wurds))
            continue;
    }
}

int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line)
{
    memset(wurds, 0, NUM_WURDS * SIZE_OF_WURD);
    int num_wurds = 0;
    char const *sep = " ";
    char *tok, *last_s;
    for (tok = strtok_r(line, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s))
    {
        strncpy(wurds[num_wurds++], tok, SIZE_OF_WURD);
        if (num_wurds == NUM_WURDS)
            break;
    }
    return num_wurds;
}

void char_array_to_seq_string_pattern(sequencer *seq, char *dest_pattern,
                                      char char_array[NUM_WURDS][SIZE_OF_WURD],
                                      int start, int end)
{
    if (strncmp("all24", char_array[start], 5) == 0)
    {
        strncat(dest_pattern,
                "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23",
                150);
    }
    else if (strncmp("all", char_array[start], 3) == 0)
    {
        if (seq->pattern_len == 24)
        {
            strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 "
                                  "15 16 17 18 19 20 21 22 23",
                    150);
        }
        else
        {
            strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", 127);
        }
    }
    else if (strncmp("none", char_array[start], 4) == 0)
    {
        // no-op
    }
    else
    {
        for (int i = start; i < end; i++)
        {
            strcat(dest_pattern, char_array[i]);
            if (i != (end - 1))
                strcat(dest_pattern, " ");
        }
    }
}

int exxit()
{
    printf(COOL_COLOR_PINK
           "\nBeat it, ya val jerk!\n" ANSI_COLOR_RESET); // Thrashin' reference
    pa_teardown();
    exit(0);
}
