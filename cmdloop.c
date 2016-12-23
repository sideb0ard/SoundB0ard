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
        if (strncmp("help", wurds[0], 4) == 0) {
            print_help();
        }

        else if (strncmp("quit", wurds[0], 4) == 0 ||
                 strncmp("exit", wurds[0], 4) == 0) {
            exxit();
        }

        else if (strncmp("bpm", wurds[0], 3) == 0) {
            int bpm = atoi(wurds[1]);
            if (bpm > 0)
                mixer_update_bpm(mixr, bpm);
        }

        else if (strncmp("ps", wurds[0], 2) == 0) {
            mixer_ps(mixr);
        }

        else if (strncmp("ls", wurds[0], 2) == 0) {
            list_sample_dir();
        }

        else if (strncmp("record", wurds[0], 6) == 0) {
            printf("Toggling record ..(NOT IMPLEMENTED)\n");
        }

        // open / save
        else if (strncmp("open", wurds[0], 4) == 0) {
            printf("Opening project ..(NOT IMPLEMENTED)\n");
        }
        else if (strncmp("save", wurds[0], 4) == 0) {
            printf("Saving project as ..(NOT IMPLEMENTED)\n");
        }

        // start / stop
        else if (strncmp("start", wurds[0], 4) == 0) {
            printf("Starting... (NOT IMPLEMENTED)\n");
        }
        else if (strncmp("stop", wurds[0], 4) == 0) {
            printf("Stopping...(NOT IMPLEMENTED)\n");
        }

        else if (strncmp("down", wurds[0], 4) == 0 ||
                 strncmp("up", wurds[0], 3) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                SBMSG *msg = new_sbmsg();
                msg->sound_gen_num = soundgen_num;
                if (strcmp("up", wurds[0]) == 0) {
                    strncpy(msg->cmd, "fadeuprrr", 19);
                }
                else {
                    strncpy(msg->cmd, "fadedownrrr", 19);
                }
                thrunner(msg);
            }
        }

        else if (strncmp("vol", wurds[0], 3) == 0) {
            if (strncmp("mixer", wurds[1], 5) == 0) {
                double vol = atof(wurds[2]);
                mixer_vol_change(mixr, vol);
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (is_valid_soundgen_num(soundgen_num)) {
                    double vol = atof(wurds[2]);
                    vol_change(mixr, soundgen_num, vol);
                }
            }
        }

        //////  STEP SEQUENCER COMMANDS  /////////////////////////
        else if (strncmp("seq", wurds[0], 3) == 0) {
            if (is_valid_file(wurds[1])) {

                char *pattern = calloc(128, sizeof(char));

                char_array_to_seq_string_pattern(pattern, wurds, 2, num_wurds);
                add_drum_char_pattern(mixr, wurds[1], pattern);

                free(pattern);
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (is_valid_soundgen_num(soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type == DRUM_TYPE) {

                    char *pattern = calloc(128, sizeof(char));
                    char_array_to_seq_string_pattern(pattern, wurds, 3,
                                                     num_wurds);

                    if (strncmp("add", wurds[2], 3) == 0) {
                        printf("Adding\n");
                        add_char_pattern(mixr->sound_generators[soundgen_num],
                                         pattern);
                    }
                    else if (strncmp("change", wurds[2], 6) == 0) {
                        printf("Changing\n");
                        change_char_pattern(
                            mixr->sound_generators[soundgen_num], pattern);
                    }
                    else if (strncmp("euclid", wurds[2], 6) == 0) {
                        int num_beats = atoi(wurds[3]);
                        int euclidean_pattern = create_euclidean_rhythm(
                            num_beats, DRUM_PATTERN_LEN);
                        bool start_at_zero =
                            strncmp("true", wurds[4], 4) == 0 ? true : false;
                        if (start_at_zero)
                            euclidean_pattern = shift_bits_to_leftmost_position(
                                euclidean_pattern, DRUM_PATTERN_LEN);
                        // TODO - this could be an add or a change
                        change_int_pattern(mixr->sound_generators[soundgen_num],
                                           euclidean_pattern);
                    }
                    else if (strncmp("life", wurds[2], 4) == 0) {
                        printf("Toggling game of life for %d\n", soundgen_num);
                        DRUM *d = (DRUM *)mixr->sound_generators[soundgen_num];
                        d->game_of_life_on = 1 - d->game_of_life_on;
                    }
                    else if (strncmp("swing", wurds[2], 5) == 0) {
                        int swing_setting = atoi(wurds[3]);
                        swingrrr(mixr->sound_generators[soundgen_num],
                                 swing_setting);
                    }

                    free(pattern);
                }
            }
        }
        // SAMPLE LOOPER COMMANDS
        else if (strncmp("loop", wurds[0], 4) == 0) {
            if (is_valid_file(wurds[1])) {
                int loop_len = atoi(wurds[2]);
                if (loop_len > 0) {
                    add_sampler(mixr, wurds[1], loop_len);
                }
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (is_valid_soundgen_num(soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        SAMPLER_TYPE) {

                    if (strncmp("change", wurds[2], 6) == 0) {
                        if (strncmp("loop_len", wurds[3], 8) == 0) {
                            SAMPLER *s =
                                (SAMPLER *)mixr->sound_generators[soundgen_num];
                            sampler_change_loop_len(s, atoi(wurds[4]));
                        }
                    }
                }
            }
        }
        // SINGLE SHOT SAMPLE PLAYER COMMANDS
        else if (strncmp("play", wurds[0], 4) == 0) {
            printf("Playing onetime sample...\n");
        }
        // SYNTHESIZER COMMANDS
        else if (strncmp("syn", wurds[0], 3) == 0) {
            printf("Synthesizing ...\n");
            if (strncmp("nano", wurds[1], 4) == 0) {
                add_nanosynth(mixr);
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (is_valid_soundgen_num(soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        NANOSYNTH_TYPE) {
                    if (strncmp("keys", wurds[2], 4) == 0) {
                        keys(soundgen_num);
                    }
                    else if (strncmp("midi", wurds[2], 4) == 0) {
                        mixr->midi_control_destination = NANOSYNTH;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else if (strncmp("reset", wurds[2], 5) == 0) {
                        nanosynth *ns =
                            (nanosynth *)mixr->sound_generators[soundgen_num];
                        nanosynth_reset_melody(ns);
                    }
                    else if (strncmp("change", wurds[2], 6) == 0) {
                        // change
                    }
                }
            }
        }
        // FX COMMANDS
        else if (strncmp("repeat", wurds[0], 6) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                int loop_len = atoi(wurds[2]);
                if (loop_len > 0) {
                    add_beatrepeat_soundgen(
                        mixr->sound_generators[soundgen_num], loop_len);
                }
            }
        }

        else if (strncmp("fx", wurds[0], 2) == 0) {
            printf("Adding/changing FX ...\n");
        }
        else {
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
    for (tok = strtok_r(line, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s)) {
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
    else if (strncmp("none", char_array[start], 4) == 0) {
        // no-op
    }
    else {
        for (int i = start; i < end; i++) {
            strcat(dest_pattern, char_array[i]);
            if (i != (end - 1))
                strcat(dest_pattern, " ");
        }
    }
}

bool is_valid_soundgen_num(int soundgen_num)
{
    if (soundgen_num >= 0 && soundgen_num < mixr->soundgen_num) {
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
    char full_filename[strlen(cwd) + strlen(filename) + strlen(subdir)];
    strcpy(full_filename, cwd);
    strcat(full_filename, subdir);
    strcat(full_filename, filename);

    printf("Is valid? %s\n", full_filename);

    struct stat buffer;
    return (stat(full_filename, &buffer) == 0);
}

int exxit()
{
    printf(COOL_COLOR_GREEN "\nBeat it, ya val jerk...\n" ANSI_COLOR_RESET);
    pa_teardown();
    exit(0);
}
