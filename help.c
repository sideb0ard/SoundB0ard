#include "help.h"
#include "defjams.h"
#include <stdio.h>

void print_help()
{
    printf(COOL_COLOR_PINK "\n");
    printf("#### SBShell - Interactive, scriptable, algorithmic music shell  "
           "####\n");

    printf("\n" COOL_COLOR_GREEN);
    printf("[Global Cmds]\n");
    printf(ANSI_COLOR_WHITE "stop" ANSI_COLOR_CYAN " -- stop playback\n");
    printf(ANSI_COLOR_WHITE "start" ANSI_COLOR_CYAN " -- re-start playback\n");
    printf(ANSI_COLOR_WHITE "bpm <bpm>" ANSI_COLOR_CYAN
                            " -- change bpm to <bpm>\n");
    printf(ANSI_COLOR_WHITE "vol <vol>" ANSI_COLOR_CYAN
                            " -- change volume to <vol>\n");
    printf(ANSI_COLOR_WHITE "ps" ANSI_COLOR_CYAN
                            " -- show global status and process listing\n");
    printf(ANSI_COLOR_WHITE
           "ls" ANSI_COLOR_CYAN
           " -- show file listing of samples, loops, and file projects\n");
    printf(ANSI_COLOR_WHITE
           "save <filename>" ANSI_COLOR_CYAN
           " -- save current project settings as <filename>\n");
    printf(ANSI_COLOR_WHITE
           "open <filename>" ANSI_COLOR_CYAN
           " -- open <filename> as current project. (Overrides "
           "current)\n");
    printf(ANSI_COLOR_WHITE "record <on/off>" ANSI_COLOR_CYAN
                            " -- toggle global record on or off\n");
    printf(ANSI_COLOR_WHITE "help" ANSI_COLOR_CYAN " -- this message, duh\n");

    printf("\n" COOL_COLOR_GREEN);
    printf("[Sound Generator Cmds]\n");
    printf(ANSI_COLOR_WHITE);
    printf(ANSI_COLOR_WHITE "seq <sample> <pattern>" ANSI_COLOR_CYAN
                            " e.g. \"seq kick2.wav 0 4 8 12\"\n");
    printf(ANSI_COLOR_WHITE "seq <sound_gen_no> add <pattern>" ANSI_COLOR_CYAN
                            " e.g. seq 0 add 4 6 10 12\n");
    printf(ANSI_COLOR_WHITE "seq <sound_gen_no> change <pattern>\n");
    printf(ANSI_COLOR_WHITE
           "seq <sound_gen_no> euclid <num_hits> [true]" ANSI_COLOR_CYAN
           "\n   -- generates "
           "equally spaced number of beats. Optional 'true' shifts\n    them "
           "forward so first hit is on first tick of cycle.\n");
    printf(ANSI_COLOR_WHITE "seq <sound_gen_no> life" ANSI_COLOR_CYAN
                            " - generative changing pattern, based on "
                            "game of life\n");
    printf(ANSI_COLOR_WHITE
           "seq <sound_gen_no> swing <swing_setting>" ANSI_COLOR_CYAN
           "\n    -- toggles swing"
           " on/off. Setting can be between 1..6, which represent\n"
           "    50%%, 54%%, 58%%, 62%%, 66%%, 71%%\n");

    printf("\n");
    printf(ANSI_COLOR_WHITE "loop <sample> <bars>" ANSI_COLOR_CYAN
                            " e.g. \"loop amen.wav 2\"\n");
    printf(ANSI_COLOR_WHITE "loop <sound_gen_no> add <sample> <bars>\n");
    printf(ANSI_COLOR_WHITE "loop <sound_gen_no> change <parameter> <val>\n");
    printf("\n");
    printf(ANSI_COLOR_WHITE "play <sample> [16th]" ANSI_COLOR_CYAN
                            " -- optional 16th, otherwise plays immediately\n");
    printf("\n");
    printf(ANSI_COLOR_WHITE "syn nano\n");
    printf(ANSI_COLOR_WHITE "syn korg\n");
    printf(ANSI_COLOR_WHITE "syn poly\n");
    printf(ANSI_COLOR_WHITE "syn <sound_gen_no> keys\n");
    printf(ANSI_COLOR_WHITE "syn <sound_gen_no> midi\n");
    printf(ANSI_COLOR_WHITE "syn <sound_gen_no> change <parameter> <val>\n");
    printf("\n" COOL_COLOR_GREEN);
    printf("[FX Cmds]\n");
    printf(ANSI_COLOR_WHITE);
    printf(ANSI_COLOR_WHITE "delay   <sound_gen_number> [off]\n");
    printf(ANSI_COLOR_WHITE "distort <sound_gen_number> [off]\n");
    printf(ANSI_COLOR_WHITE "reverb  <sound_gen_number> [off]\n");
    printf(ANSI_COLOR_WHITE "crush   <sound_gen_number> [off]\n");
    printf(ANSI_COLOR_WHITE "repeat  <sound_gen_number> [off]\n");
    printf("\n");
    printf(ANSI_COLOR_WHITE "fx <sound_gen_number> change <parameter> <val>\n");

    printf(ANSI_COLOR_RESET);
}
