#include "help.h"
#include "defjams.h"
#include <stdio.h>

void print_help()
{
    printf(COOL_COLOR_PINK"\n");
    printf("Haaaaalp!\n");
    printf("#### SBShell - Interactive, scriptable, algorithmic music shell  ####\n");

    printf("\n"COOL_COLOR_GREEN);
    printf("[Global Cmds]\n");
    printf(ANSI_COLOR_WHITE);
    printf("stop -- stop playback\n");
    printf("start -- re-start playback\n");
    printf("bpm <bpm> -- change bpm to <bpm>\n");
    printf("vol <vol> -- change volume to <vol>\n");
    printf("ps -- show global status and process listing\n");
    printf("ls -- show file listing of samples, loops, and file projects\n");
    printf("save <filename> -- save current project settings as <filename>\n");
    printf("open <filename> -- open <filename> as current project. (Overrides current)\n");
    printf("record <on/off> -- toggle global record on or off\n");
    printf("help -- this message, duh\n");

    printf("\n"COOL_COLOR_GREEN);
    printf("[Sound Generator Cmds]\n");
    printf(ANSI_COLOR_WHITE);
    printf("seq <sample> <pattern> e.g. \"seq kick2.wav 0 4 8 12\"\n");
    printf("seq <sound_gen_no> add <pattern> e.g. seq 0 add 4 6 10 12\n");
    printf("seq <sound_gen_no> change <parameter> <val>\n");
    printf("\n");
    printf("loop <sample> <bars> e.g. \"loop amen.wav 2\"\n");
    printf("loop <sound_gen_no> add <sample> <bars>\n");
    printf("loop <sound_gen_no> change <parameter> <val>\n");
    printf("\n");
    printf("play <sample> [16th] -- optional 16th, otherwise plays immediately\n");
    printf("\n");
    printf("syn nano\n");
    printf("syn korg\n");
    printf("syn poly\n");
    printf("syn <sound_gen_no> keys\n");
    printf("syn <sound_gen_no> midi\n");
    printf("syn <sound_gen_no> change <parameter> <val>\n");
    printf("\n"COOL_COLOR_GREEN);
    printf("[FX Cmds]\n");
    printf(ANSI_COLOR_WHITE);
    printf("delay   <sound_gen_number> [off]\n");
    printf("distort <sound_gen_number> [off]\n");
    printf("reverb  <sound_gen_number> [off]\n");
    printf("crush   <sound_gen_number> [off]\n");
    printf("repeat  <sound_gen_number> [off]\n");
    printf("\n");
    printf("fx <sound_gen_number> change <parameter> <val>\n");

    printf(ANSI_COLOR_RESET);

}
