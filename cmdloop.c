#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "audioutils.h"
#include "beatrepeat.h"
#include "cmdloop.h"
#include "defjams.h"
#include "drumr_utils.h"
#include "envelope.h"
#include "help.h"
#include "keys.h"
#include "midimaaan.h"
#include "mixer.h"
#include "nanosynth.h"
#include "sampler.h"
#include "table.h"
#include "utils.h"

extern mixer *mixr;

extern wtable *wave_tables[5];

void loopy(void)
{
    char *line;
    while ((line = readline(ANSI_COLOR_MAGENTA "SB#> " ANSI_COLOR_RESET)) !=
           NULL) {
        if (line[0] != 0) {
            add_history(line);
            interpret(line);
        }
        free(line);
    }
}

void interpret(char *line)
{
    char wurds[NUM_WURDS][SIZE_OF_WURD];

    char *cmd, *last_s;
    char *sep = ",";
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s)) {

        int num_wurds = parse_wurds_from_cmd(wurds, cmd);

        //////  MIXER COMMANDS  /////////////////////////
        if (strncmp(wurds[0], "help", SIZE_OF_WURD) == 0)
        {
            print_help();
        }

        else if (strncmp(wurds[0], "quit", SIZE_OF_WURD) == 0
                || strncmp(wurds[0], "quit", SIZE_OF_WURD) == 0)
        {
            exxit();

        }

        else if (strncmp(wurds[0], "bpm", SIZE_OF_WURD) == 0)
        {
            int bpm = atoi(wurds[1]);
            if ( bpm > 0 )
                mixer_update_bpm(mixr, bpm);
        }

        else if (strncmp(wurds[0], "vol", SIZE_OF_WURD) == 0)
        {
            double vol = atof(wurds[1]);
            mixer_vol_change(mixr, vol);
        }

        else if (strncmp(wurds[0], "ps", SIZE_OF_WURD) == 0)
        {
            mixer_ps(mixr);
        }

        else if (strncmp(wurds[0], "ls", SIZE_OF_WURD) == 0)
        {
            list_sample_dir();
        }

        else if (strncmp(wurds[0], "record", SIZE_OF_WURD) == 0)
        {
            printf("Toggling record ..(NOT IMPLEMENTED)\n");
        }

        // open / save
        else if (strncmp(wurds[0], "open", SIZE_OF_WURD) == 0)
        {
            printf("Opening project ..(NOT IMPLEMENTED)\n");
        }
        else if (strncmp(wurds[0], "save", SIZE_OF_WURD) == 0)
        {
            printf("Saving project as ..(NOT IMPLEMENTED)\n");
        }

        // start / stop
        else if (strncmp(wurds[0], "start", SIZE_OF_WURD) == 0)
        {
            printf("Starting... (NOT IMPLEMENTED)\n");
        }
        else if (strncmp(wurds[0], "stop", SIZE_OF_WURD) == 0)
        {
            printf("Stopping...(NOT IMPLEMENTED)\n");
        }

        //////  SOUND GENERATOR COMMANDS  /////////////////////////
        else if (strncmp(wurds[0], "seq", SIZE_OF_WURD) == 0)
        {
            if ( is_valid_file(wurds[1]) ) {

                char *pattern = calloc(128, sizeof(char));

                char_array_to_seq_string_pattern(pattern, wurds, 2, num_wurds); 
                add_drum_char_pattern(mixr, wurds[1], pattern);

                free(pattern);
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (is_valid_soundgen_num(soundgen_num)
                    && mixr->sound_generators[soundgen_num]->type == DRUM_TYPE)
                {

                    char *pattern = calloc(128, sizeof(char));
                    char_array_to_seq_string_pattern(pattern, wurds, 3, num_wurds); 

                    if (strncmp("add", wurds[2], 3) == 0) {
                        printf("Adding\n");
                        add_char_pattern(mixr->sound_generators[soundgen_num], pattern);
                    }
                    else if (strncmp("change", wurds[2], 6) == 0) {
                        printf("Changing\n");
                        change_char_pattern(mixr->sound_generators[soundgen_num], pattern);
                    }

                    free(pattern);
                }
            }
        }
        else if (strncmp(wurds[0], "loop", SIZE_OF_WURD) == 0)
        {
            printf("Looping sample...\n");
        }
        else if (strncmp(wurds[0], "play", SIZE_OF_WURD) == 0)
        {
            printf("Playing onetime sample...\n");
        }
        else if (strncmp(wurds[0], "syn", SIZE_OF_WURD) == 0)
        {
            printf("Synthesizing ...\n");
        }
        else if (strncmp(wurds[0], "fx", SIZE_OF_WURD) == 0)
        {
            printf("Adding/changing FX ...\n");
        }
        else
        {
            print_help();
        }
    }
}

int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line)
{
    memset(wurds, 0, NUM_WURDS * SIZE_OF_WURD);
    int num_wurds = 0;
    char *sep = " ";
    char *tok, *last_s;
    for (tok = strtok_r(line, sep, &last_s);
         tok;
         tok = strtok_r(NULL, sep, &last_s)) 
    {
        strncpy(wurds[num_wurds++], tok, SIZE_OF_WURD);
        if (num_wurds == NUM_WURDS)
            break;
    }
    return num_wurds;
}

void char_array_to_seq_string_pattern(char *dest_pattern,
                                      char char_array[NUM_WURDS][SIZE_OF_WURD],
                                      int start, int end)
{
    if (strncmp("all", char_array[start], 3) == 0) {
        strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", 127);
    }
    else if (strncmp("none", char_array[start], 4) == 0)
    {
        // no-op
    }
    else
    {
        for (int i = start; i < end; i++) {
            strcat(dest_pattern, char_array[i]);
            if ( i != (end - 1) )
                strcat(dest_pattern, " ");
        }
    }
}

bool is_valid_soundgen_num(int soundgen_num)
{
    if ( soundgen_num >= 0 && soundgen_num < mixr->soundgen_num) {
        return true;
    }
    return false;
}

bool is_valid_file(char *filename)
{
    printf("Padding up %s\n", filename);
    char cwd[1024];
    getcwd(cwd, 1024);
    char *subdir = "/wavs/";
    char full_filename[strlen(cwd)
                       + strlen(filename)
                       + strlen(subdir)];
    strcpy(full_filename, cwd);
    strcat(full_filename, subdir);
    strcat(full_filename, filename);

    printf("Is valid? %s\n", full_filename);

    struct stat buffer;   
    return (stat (full_filename, &buffer) == 0);
}

int exxit()
{
    printf(COOL_COLOR_GREEN "\nBeat it, ya val jerk...\n" ANSI_COLOR_RESET);
    pa_teardown();
    exit(0);
}

