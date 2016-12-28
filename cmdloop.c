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

#include "algorithm.h"
#include "audioutils.h"
#include "beatrepeat.h"
#include "chaosmonkey.h"
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

char *prompt = ANSI_COLOR_MAGENTA "SB#> " ANSI_COLOR_RESET;

void print_prompt()
{
    printf("%s", prompt);
    fflush(stdout);
}

void loopy(void)
{
    char *line;
    while ((line = readline(prompt)) !=
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

        char tmp[128];
        strncpy((char *)tmp, cmd, 127);

        int num_wurds = parse_wurds_from_cmd(wurds, tmp);

        // Understanding these commands should make more sense when viewed
        // against the help output, which details each command
        // (TODO) some kid of doxygen-like scheme to write help output here

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
                        // https://en.wikipedia.org/wiki/Euclidean_rhythm
                        int num_beats = atoi(wurds[3]);
                        if (num_beats <= 0) {
                            // TODO - make debug - printf("Need a number of
                            // beats\n");
                            return;
                        }
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

                    if (strncmp("add", wurds[2], 6) == 0) {
                        if (is_valid_file(wurds[3])) {
                            int loop_len = atoi(wurds[4]);
                            if (loop_len > 0) {
                                sampler_add_sample(
                                    (SAMPLER *)
                                        mixr->sound_generators[soundgen_num],
                                    wurds[3], loop_len);
                            }
                        }
                    }
                    else if (strncmp("change", wurds[2], 6) == 0) {
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
                    else if (strncmp("sustain", wurds[2], 7) == 0) {
                        nanosynth *ns =
                            (nanosynth *)mixr->sound_generators[soundgen_num];
                        int val = atoi(wurds[3]);
                        nanosynth_set_sustain(ns,
                                              val * mixr->midi_ticks_per_ms);
                    }
                    else if (strncmp("change", wurds[2], 6) == 0) {
                        // change
                    }
                }
            }
        }

        // CHAOS COMMANDS
        else if (strncmp("chaos", wurds[0], 6) == 0) {

            if (strncmp("monkey", wurds[1], 6) == 0) {
                add_chaosmonkey();
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (is_valid_soundgen_num(soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        CHAOSMONKEY_TYPE) {

                    chaosmonkey *cm =
                        (chaosmonkey *)mixr->sound_generators[soundgen_num];

                    if (strncmp("wakeup", wurds[2], 7) == 0) {
                        int freq_wakeup_secs = atoi(wurds[3]);
                        chaosmonkey_change_wakeup_freq(cm, freq_wakeup_secs);
                    }
                    else if (strncmp("chance", wurds[2], 7) == 0) {
                        int percent_chance = atoi(wurds[3]);
                        chaosmonkey_change_chance_interrupt(cm, percent_chance);
                    }
                    else if (strncmp("suggest", wurds[2], 8) == 0) {
                        if (strncmp("true", wurds[3], 5) == 0 ||
                            strncmp("false", wurds[3], 6) == 0) {
                            bool val =
                                strcmp("true", wurds[4]) == 0 ? true : false;
                            chaosmonkey_suggest_mode(cm, val);
                        }
                    }
                    else if (strncmp("action", wurds[2], 7) == 0) {
                        if (strncmp("true", wurds[3], 5) == 0 ||
                            strncmp("false", wurds[3], 6) == 0) {
                            bool val =
                                strcmp("true", wurds[4]) == 0 ? true : false;
                            chaosmonkey_action_mode(cm, val);
                        }
                    }
                }
            }
        }
        // FX COMMANDS
        else if (strncmp("delay", wurds[0], 7) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                int delay_len_ms = atoi(wurds[2]);
                add_delay_soundgen(mixr->sound_generators[soundgen_num], delay_len_ms);
            }
        }
        else if (strncmp("distort", wurds[0], 7) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                add_distortion_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("decimate", wurds[0], 8) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                add_decimator_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("env", wurds[0], 3) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                int loop_len = atoi(wurds[2]);
                int env_type = atoi(wurds[3]);
                ENVSTREAM *e = new_envelope_stream(loop_len, env_type);
                if (e != NULL) {
                    add_envelope_soundgen(mixr->sound_generators[soundgen_num],
                                          e);
                }
            }
        }
        else if (strncmp("lowpass", wurds[0], 8) == 0 ||
                 strncmp("highpass", wurds[0], 9) == 0 ||
                 strncmp("bandpass", wurds[0], 9) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                int val = atoi(wurds[2]);
                if (strcmp("lowpass", wurds[0]) == 0)
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, LOWPASS);
                else if (strcmp("highass", wurds[0]) == 0)
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, HIGHPASS);
                else
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, BANDPASS);
            }
        }

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
        else if (strncmp("sidechain", wurds[0], 9) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (is_valid_soundgen_num(soundgen_num)) {
                int input_src = atoi(wurds[2]);
                int percent_mix = atoi(wurds[3]);
                printf("SIDEHCINA %d %d %d\n", soundgen_num, input_src,
                       percent_mix);
                // if (mixr->sound_generators[input_src]->type == DRUM_TYPE) {
                //    DRUM *d = (DRUM *)
                //    mixr->sound_generators[input_src]->type;
                //    int pat_array[DRUM_PATTERN_LEN];
                //    int_pattern_to_array(d->patterns[d->cur_pattern_num],
                //                         pat_array);
                //    for (int i = 0; i < DRUM_PATTERN_LEN; i++)
                //    {
                //        printf("DRUMMMMM %d\n", pat_array[0]);
                //    }
                //    //ENVSTREAM *e = new_sidechain_stream(pat_array,
                //    percent_mix);
                //    //printf("GOT STREAM\n");
                //    //add_envelope_soundgen(mixr->sound_generators[soundgen_num],
                //    e);
                //}
            }
        }

        else if (strncmp("fx", wurds[0], 2) == 0) {
            int soundgen_num = atoi(wurds[1]);
            int fx_num = atoi(wurds[2]);
            if (is_valid_fx_num(soundgen_num, fx_num)) {
                if (strncmp("nbeats", wurds[3], 6) == 0 ||
                    strncmp("16th", wurds[3], 4) == 0) {
                    if (mixr->sound_generators[soundgen_num]
                            ->effects[fx_num]
                            ->type == BEATREPEAT) {
                        beatrepeat *b =
                            (beatrepeat *)mixr->sound_generators[soundgen_num]
                                ->effects[fx_num];
                        if (strncmp("nbeats", wurds[3], 6) == 0) {
                            int nbeats = atoi(wurds[4]);
                            beatrepeat_change_num_beats_to_repeat(b, nbeats);
                        }
                        else {
                            int s16th = atoi(wurds[4]);
                            beatrepeat_change_selected_sixteenth(b, s16th);
                        }
                    }
                }
                else if (strncmp("midi", wurds[3], 4) == 0) {
                    if (mixr->sound_generators[soundgen_num]
                            ->effects[fx_num]
                            ->type == DELAY) {
                        printf("SUCCESS! GOLDEN MIDI DELAY!\n");
                        mixr->midi_control_destination = DELAYFX;
                        mixr->active_midi_soundgen_num = soundgen_num;
                        mixr->active_midi_soundgen_effect_num = fx_num;
                    }
                }
                else if (strncmp("delay", wurds[3], 5) == 0 ||
                         strncmp("feedback", wurds[3], 8) == 0 ||
                         strncmp("ratio", wurds[3], 5) == 0 ||
                         strncmp("mix", wurds[3], 3) == 0 ||
                         strncmp("mode", wurds[3], 4) == 0) {
                    if (mixr->sound_generators[soundgen_num]
                            ->effects[fx_num]
                            ->type == DELAY) {
                        stereodelay *d =
                            (stereodelay *)mixr->sound_generators[soundgen_num]
                                ->effects[fx_num];
                        if (strncmp("delay", wurds[3], 5) == 0) {
                            printf("Changing DELAY TIME\n");
                            double delay_ms = atof(wurds[4]);
                            delay_set_delay_time_ms(d, delay_ms);
                        }
                        else if (strncmp("feedback", wurds[3], 8) == 0) {
                            printf("Changing FEEDBACK TIME\n");
                            int percent = atoi(wurds[4]);
                            delay_set_feedback_percent(d, percent);
                        }
                        else if (strncmp("ratio", wurds[3], 5) == 0) {
                            printf("Changing RATIO TIME\n");
                            double ratio = atof(wurds[4]);
                            delay_set_delay_ratio(d, ratio);
                        }
                        else if (strncmp("mix", wurds[3], 3) == 0) {
                            printf("Changing MIX TIME\n");
                            double mix = atof(wurds[4]);
                            delay_set_wet_mix(d, mix);
                        }
                        else if (strncmp("mode", wurds[3], 4) == 0) {
                            printf("MODE!\n");
                            if (strncmp("NORM", wurds[4], 4) == 0) {
                                delay_set_mode(d, NORM);
                            }
                            else if (strncmp("TAP1", wurds[4], 4) == 0) {
                                delay_set_mode(d, TAP1);
                            }
                            else if (strncmp("TAP2", wurds[4], 4) == 0) {
                                delay_set_mode(d, TAP2);
                            }
                            else if (strncmp("PINGPONG", wurds[4], 8) == 0) {
                                delay_set_mode(d, PINGPONG);
                            }
                        }
                    }
                }
            }
        }

        // PROGRAMMING CMDS
        else if (strncmp("var", wurds[0], 3) == 0 &&
                 strncmp("=", wurds[2], 1) == 0) {
            printf("Oooh! %s = %s\n", wurds[1], wurds[3]);
            update_environment(wurds[1], atoi(wurds[3]));
        }

        else if (strncmp("every", wurds[0], 5) == 0 &&
                 strncmp("loop", wurds[1], 4) == 0) {
            printf("Starting an algorithm - with %s!\n", cmd);
            add_algorithm(cmd);
        }

        // UTILS
        else if (strncmp("chord", wurds[0], 6) == 0) {
            chordie(wurds[1]);
        }

        else if (strncmp("strategy", wurds[0], 8) == 0) {
            oblique_strategy();
        }

        // default HALP!
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

bool is_valid_fx_num(int soundgen_num, int fx_num)
{
    if (is_valid_soundgen_num(soundgen_num)) {
        if (mixr->sound_generators[soundgen_num]->effects_num > 0 &&
            fx_num < mixr->sound_generators[soundgen_num]->effects_num) {
            return true;
        }
    }
    return false;
}

bool is_valid_file(char *filename)
{
    char cwd[1024];
    getcwd(cwd, 1024);
    char *subdir = "/wavs/";
    char full_filename[strlen(cwd) + strlen(filename) + strlen(subdir)];
    strcpy(full_filename, cwd);
    strcat(full_filename, subdir);
    strcat(full_filename, filename);

    struct stat buffer;
    return (stat(full_filename, &buffer) == 0);
}

int exxit()
{
    printf(COOL_COLOR_GREEN "\nBeat it, ya val jerk...\n" ANSI_COLOR_RESET);
    pa_teardown();
    exit(0);
}
