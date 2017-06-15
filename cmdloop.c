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
#include "audioutils.h"
#include "beatrepeat.h"
#include "chaosmonkey.h"
#include "cmdloop.h"
#include "defjams.h"
#include "envelope.h"
#include "help.h"
#include "keys.h"
#include "midimaaan.h"
#include "minisynth.h"
#include "mixer.h"
#include "modfilter.h"
#include "modular_delay.h"
#include "obliquestrategies.h"
#include "oscillator.h"
#include "reverb.h"
#include "sample_sequencer.h"
#include "sequencer_utils.h"
#include "sparkline.h"
#include "spork.h"
#include "synthdrum_sequencer.h"
#include "table.h"
#include "utils.h"

extern mixer *mixr;

extern wtable *wave_tables[5];

char const *prompt = ANSI_COLOR_MAGENTA "SB#> " ANSI_COLOR_RESET;

void print_prompt()
{
    printf("%s", prompt);
    fflush(stdout);
}

void loopy(void)
{
    setlocale(LC_ALL, "");

    char *line;
    while ((line = readline(prompt)) != NULL) {
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
    char const *sep = ",";
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

        else if (strncmp("new", wurds[0], 3) == 0) {
            if (strncmp("spork", wurds[1], 5) == 0) {
                printf("Sp0rky!\n");
                double freq = atof(wurds[2]);
                if (freq > 0.) {
                    mixer_add_spork(mixr, freq);
                }
                else {
                    mixer_add_spork(mixr, 440);
                }
            }
        }

        else if (strncmp("debug", wurds[0], 5) == 0) {
            if (strncmp("on", wurds[1], 2) == 0 ||
                strncmp("true", wurds[1], 4) == 0) {
                printf("Enabling debug mode\n");
                mixr->debug_mode = true;
            }
            else if (strncmp("off", wurds[1], 2) == 0 ||
                     strncmp("false", wurds[1], 5) == 0) {
                printf("Disabling debug mode\n");
                mixr->debug_mode = false;
            }
        }

        else if (strncmp("ps", wurds[0], 2) == 0) {
            mixer_ps(mixr);
        }

        else if (strncmp("quiet", wurds[0], 5) == 0) {
            for (int i = 0; i < mixr->soundgen_num; i++)
                mixr->sound_generators[i]->setvol(mixr->sound_generators[i],
                                                  0.0);
        }
        else if (strncmp("ls", wurds[0], 2) == 0) {
            list_sample_dir();
        }

        else if (strncmp("rm", wurds[0], 3) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                printf("Deleting SOUND GEN %d\n", soundgen_num);
                mixer_del_soundgen(mixr, soundgen_num);
            }
        }
        else if (strncmp("start", wurds[0], 5) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                printf("Starting SOUND GEN %d\n", soundgen_num);
                SOUNDGEN *sg = mixr->sound_generators[soundgen_num];
                sg->start(sg);
            }
        }
        else if (strncmp("stop", wurds[0], 5) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                printf("Stopping SOUND GEN %d\n", soundgen_num);
                SOUNDGEN *sg = mixr->sound_generators[soundgen_num];
                sg->stop(sg);
            }
        }
        else if (strncmp("down", wurds[0], 4) == 0 ||
                 strncmp("up", wurds[0], 3) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
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
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                    double vol = atof(wurds[2]);
                    vol_change(mixr, soundgen_num, vol);
                }
            }
        }

        else if (strncmp("scene", wurds[0], 5) == 0) {
            if (strncmp("new", wurds[1], 4) == 0) {
                int num_bars = atoi(wurds[2]);
                if (num_bars == 0)
                    num_bars = 4; // default
                int new_scene_num = mixer_add_scene(mixr, num_bars);
                printf("New scene %d with %d bars!\n", new_scene_num, num_bars);
            }
            else if (strncmp("mode", wurds[1], 4) == 0) {
                if (strncmp("on", wurds[2], 2) == 0) {
                    mixr->scene_mode = true;
                    mixr->scene_start_pending = true;
                }
                else if (strncmp("off", wurds[2], 3) == 0) {
                    mixr->scene_mode = false;
                    mixr->scene_start_pending = false;
                }
                else
                    mixr->scene_mode = 1 - mixr->scene_mode;
                printf("Mode scene! %s\n", mixr->scene_mode ? "true" : "false");
            }
            else {
                int scene_num = atoi(wurds[1]);
                if (mixer_is_valid_scene_num(mixr, scene_num)) {
                    printf("Changing scene %d\n", scene_num);
                    if (strncmp("add", wurds[2], 3) == 0) {
                        int sg_num = atoi(wurds[3]);
                        int sg_track_num = atoi(wurds[4]);
                        printf("Gots nums %d %d\n", sg_num, sg_track_num);
                        if (mixer_is_valid_soundgen_track_num(mixr, sg_num,
                                                              sg_track_num)) {
                            printf("Adding sg %d %d\n", sg_num, sg_track_num);
                            mixer_add_soundgen_track_to_scene(
                                mixr, scene_num, sg_num, sg_track_num);
                        }
                    }
                    else if (strncmp("cp", wurds[2], 2) == 0) {
                        int scene_num2 = atoi(wurds[3]);
                        if (mixer_is_valid_scene_num(mixr, scene_num2)) {
                            printf("Copying scene %d to %d\n", scene_num,
                                   scene_num2);
                            mixer_cp_scene(mixr, scene_num, scene_num2);
                        }
                        else {
                            printf("Not copying scene %d -- %d is not a valid "
                                   "destination\n",
                                   scene_num, scene_num2);
                        }
                    }
                    else if (strncmp("dupe", wurds[2], 4) == 0) {
                        printf("Duplicating scene %d\n", scene_num);
                        int default_num_bars = 4;
                        int new_scene_num =
                            mixer_add_scene(mixr, default_num_bars);
                        mixer_cp_scene(mixr, scene_num, new_scene_num);
                    }
                    else if (strncmp("rm", wurds[2], 2) == 0) {
                        int sg_num = 0;
                        int sg_track_num = 0;
                        sscanf(wurds[3], "%d:%d", &sg_num, &sg_track_num);
                        if (mixer_is_valid_soundgen_track_num(mixr, sg_num,
                                                              sg_track_num)) {
                            printf("Removing sg %d %d\n", sg_num, sg_track_num);
                            mixer_rm_soundgen_track_from_scene(
                                mixr, scene_num, sg_num, sg_track_num);
                        }
                    }
                    else {
                        printf("Queueing scene %d\n", scene_num);
                        mixr->scene_start_pending = true;
                        mixr->current_scene = scene_num;
                    }
                }
            }
        }
        // else if (strncmp("synthdrum", wurds[0], 9) == 0) {
        //    char *pattern = (char *)calloc(128, sizeof(char));
        //    char_array_to_seq_string_pattern(pattern, wurds, 2, num_wurds);
        //    printf("Patterrnnzz! %s\n", pattern);
        //    int int_pattern = 0;
        //    pattern_char_to_int(pattern, &int_pattern);
        //    printf("Patterrnnzz! %s %d\n", pattern, int_pattern);
        //    if (strncmp("new", wurds[1], 4) == 0) {
        //        int soundgen_num = mixer_add_synthdrum(mixr, int_pattern);
        //        mixr->midi_control_destination = MIDISYNTHDRUM;
        //        mixr->active_midi_soundgen_num = soundgen_num;
        //    }
        //    if (strncmp("list", wurds[1], 4) == 0) {
        //        printf("Listing SYNTHDRUM patches.. \n");
        //        synthdrum_list_patches();
        //    }
        //    else {
        //        int soundgen_num = atoi(wurds[1]);
        //        if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
        //            mixr->sound_generators[soundgen_num]->type ==
        //                SYNTHDRUM_TYPE) {
        //            if (strncmp("midi", wurds[2], 4) == 0) {
        //                printf("MIDI goes to Da Winner .. SYNTHDRUM Sequencer
        //                %d\n",
        //                       soundgen_num);
        //                mixr->midi_control_destination = MIDISYNTHDRUM;
        //                mixr->active_midi_soundgen_num = soundgen_num;
        //            }
        //            else {
        //                synthdrum_sequencer *s =
        //                    (synthdrum_sequencer *)
        //                        mixr->sound_generators[soundgen_num];
        //                sequencer *seq = &s->m_seq;
        //                if (strncmp("open", wurds[2], 4) == 0) {
        //                    printf("Opening SYNTHDRUM patch %s\n", wurds[2]);
        //                    synthdrum_open_patch(s, wurds[3]);
        //                }
        //                if (strncmp("save", wurds[2], 4) == 0) {
        //                    printf("Saving SYNTHDRUM pattern as %s\n",
        //                           wurds[2]);
        //                    synthdrum_save_patch(s, wurds[3]);
        //                }
        //                else {
        //                    printf("SYNTHDRUM SEQ!\n");
        //                    parse_sequencer_command(seq, wurds, num_wurds,
        //                                            pattern);
        //                }
        //            }
        //        }
        //    }
        //}

        //////  STEP SEQUENCER COMMANDS  /////////////////////////
        else if (strncmp("seq", wurds[0], 3) == 0) {

            char *pattern = (char *)calloc(151, sizeof(char));

            if (is_valid_file(wurds[1])) {
                sample_sequencer *s = new_sample_seq(wurds[1]);
                char_array_to_seq_string_pattern(&s->m_seq, pattern, wurds, 2,
                                                 num_wurds);
                int sgnum = add_sound_generator(
                    mixr,
                    (SOUNDGEN *)
                        s); //  add_seq_char_pattern(mixr, wurds[1], pattern);
                pattern_char_to_pattern(
                    &s->m_seq, pattern,
                    s->m_seq.patterns[s->m_seq.num_patterns++]);
                mixr->midi_control_destination = MIDISEQUENCER;
                mixr->active_midi_soundgen_num = sgnum;
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        SEQUENCER_TYPE) {

                    if (strncmp("midi", wurds[2], 4) == 0) {
                        printf("MIDI goes to Da Winner .. Sequencer %d\n",
                               soundgen_num);
                        mixr->midi_control_destination = MIDISEQUENCER;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else {
                        sample_sequencer *s =
                            (sample_sequencer *)
                                mixr->sound_generators[soundgen_num];
                        sequencer *seq = &s->m_seq;
                        parse_sequencer_command(seq, wurds, num_wurds, pattern);
                    }
                }
            }
            free(pattern);
        }

        else if (strncmp("spork", wurds[0], 5) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                mixr->sound_generators[soundgen_num]->type == SPORK_TYPE) {
                spork *s = (spork *)mixr->sound_generators[soundgen_num];
                printf("Your Spork is my command!\n");
                double val = atof(wurds[3]);
                if (strncmp("freq", wurds[2], 4) == 0)
                    spork_set_freq(s, val);
                else if (strncmp("waveform", wurds[2], 8) == 0)
                    spork_set_waveform(s, val);
                else if (strncmp("mode", wurds[2], 4) == 0)
                    spork_set_mode(s, val);
                else if (strncmp("polarity", wurds[2], 4) == 0)
                    spork_set_polarity(s, val);
            }
        }
        // SAMPLE LOOPER COMMANDS
        else if (strncmp("loop", wurds[0], 4) == 0) {
            if (is_valid_file(wurds[1]) || strncmp(wurds[1], "none", 4) == 0) {
                int loop_len = atoi(wurds[2]);
                if (loop_len > 0) {
                    int soundgen_num = add_looper(mixr, wurds[1], loop_len);
                    printf("SOUNDGEN %d\n", soundgen_num);
                    mixr->midi_control_destination = MIDILOOPER;
                    mixr->active_midi_soundgen_num = soundgen_num;
                }
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type == LOOPER_TYPE) {

                    looper *s = (looper *)mixr->sound_generators[soundgen_num];

                    if (strncmp("add", wurds[2], 6) == 0) {
                        if (is_valid_file(wurds[3]) ||
                            strncmp(wurds[3], "none", 4) == 0) {
                            int loop_len = atoi(wurds[4]);
                            if (loop_len > 0) {
                                looper_add_sample(s, wurds[3], loop_len);
                            }
                        }
                    }
                    else if (strncmp("change", wurds[2], 6) == 0) {
                        int sample_num = atoi(wurds[3]);
                        if (is_valid_sample_num(s, sample_num)) {
                            if (strncmp("looplen", wurds[4], 8) == 0) {
                                int looplen = atoi(wurds[5]);
                                s->pending_loop_num = sample_num;
                                s->pending_loop_size = looplen;
                                s->change_loopsize_pending = true;
                                printf("CHANGEING PENDING ..\n");
                            }
                            else if (strncmp("numloops", wurds[4], 8) == 0) {
                                int numloops = atoi(wurds[5]);
                                if (numloops != 0) {
                                    looper_change_num_loops(s, sample_num,
                                                            numloops);
                                }
                            }
                        }
                    }
                    else if (strncmp("midi", wurds[2], 4) == 0) {
                        mixr->midi_control_destination = MIDILOOPER;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else if (strncmp("multi", wurds[2], 5) == 0) {
                        if (strncmp("true", wurds[3], 4) == 0) {
                            looper_set_multi_sample_mode(s, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0) {
                            looper_set_multi_sample_mode(s, false);
                        }
                        printf("Sampler multi mode : %s\n",
                               s->multi_sample_mode ? "true" : "false");
                    }
                    else if (strncmp("scramble", wurds[2], 8) == 0) {
                        if (strncmp(wurds[3], "true", 4) == 0)
                            looper_set_scramble_mode(s, true);
                        else if (strncmp(wurds[3], "false", 5) == 0)
                            looper_set_scramble_mode(s, false);
                        else {
                            int max_gen = atoi(wurds[3]);
                            if (max_gen > 0) {
                                looper_set_max_generation(s, max_gen);
                                looper_set_scramble_mode(s, true);
                            }
                            else {
                                printf("Toggling scramble..\n");
                                int new_mode = 1 - s->scramblrrr_mode;
                                looper_set_scramble_mode(s, new_mode);
                            }
                        }
                    }
                    else if (strncmp("stutter", wurds[2], 7) == 0) {
                        if (strncmp(wurds[3], "every", 4) == 0) {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0) {
                                printf("Stuttering every %d n loops!\n",
                                       num_gens);
                                looper_set_stutter_mode(s, true);
                                s->stutter_every_n_loops = num_gens;
                            }
                            else {
                                printf("Need a number for every 'n'\n");
                            }
                        }
                        else if (strncmp(wurds[3], "for", 3) == 0) {
                            int max_gen = atoi(wurds[3]);
                            if (max_gen > 0) {
                                looper_set_max_generation(s, max_gen);
                                looper_set_stutter_mode(s, true);
                            }
                            else {
                                printf("Need a number of loops for 'for'\n");
                            }
                        }
                        else if (strncmp(wurds[3], "true", 4) == 0)
                            looper_set_stutter_mode(s, true);
                        else if (strncmp(wurds[3], "false", 5) == 0)
                            looper_set_stutter_mode(s, false);
                        else {
                            int new_mode = 1 - s->stutter_mode;
                            printf("Toggling sTUTTER to %s..\n",
                                   new_mode ? "true" : "false");
                            looper_set_stutter_mode(s, new_mode);
                        }
                    }
                    else if (strncmp("switch", wurds[3], 6) == 0) {
                        int sample_num = atoi(wurds[3]);
                        looper_switch_sample(s, sample_num);
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
            if (strncmp("mini", wurds[1], 4) == 0) {
                int sgnum = add_minisynth(mixr);
                mixr->midi_control_destination = SYNTH;
                mixr->active_midi_soundgen_num = sgnum;
                if (num_wurds > 2) {
                    minisynth *ms = (minisynth *)mixr->sound_generators[sgnum];
                    char_melody_to_midi_melody(ms, 0, wurds, 2, num_wurds);
                }
            }
            if (strncmp("ls", wurds[1], 2) == 0) {
                minisynth_list_presets();
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type == SYNTH_TYPE) {
                    minisynth *ms =
                        (minisynth *)mixr->sound_generators[soundgen_num];

                    if (strncmp("add", wurds[2], 3) == 0) {
                        if (strncmp("melody", wurds[3], 6) == 0) {
                            int new_melody_num = minisynth_add_melody(ms);
                            if (num_wurds > 4) {
                                char_melody_to_midi_melody(ms, new_melody_num,
                                                           wurds, 4, num_wurds);
                            }
                        }
                    }
                    else if (strncmp("arp", wurds[2], 3) == 0) {
                        ms->recording = false;
                        minisynth_set_arpeggiate(ms, 1 - ms->m_arp.active);
                    }
                    else if (strncmp("change", wurds[2], 6) == 0) {
                        if (parse_minisynth_settings_change(ms, wurds,
                                                            num_wurds)) {
                            continue;
                        }
                        int melody_num = atoi(wurds[3]);
                        if (is_valid_melody_num(ms, melody_num)) {
                            if (strncmp("numloops", wurds[4], 8) == 0) {
                                int numloops = atoi(wurds[5]);
                                if (numloops != 0) {
                                    minisynth_set_melody_loop_num(
                                        ms, melody_num, numloops);
                                    printf("NUMLOOPS Now %d\n", numloops);
                                }
                            }
                            else if (strncmp("add", wurds[4], 3) == 0) {
                                int tick = 0;
                                int midi_note = 0;
                                sscanf(wurds[5], "%d:%d", &tick, &midi_note);
                                if (midi_note != 0) {
                                    printf("Adding note\n");
                                    minisynth_add_note(ms, melody_num, tick,
                                                       midi_note);
                                }
                            }
                            else if (strncmp("mv", wurds[4], 2) == 0) {
                                int fromtick = atoi(wurds[5]);
                                int totick = atoi(wurds[6]);
                                printf("MV'ing note\n");
                                minisynth_mv_note(ms, melody_num, fromtick,
                                                  totick);
                            }
                            else if (strncmp("rm", wurds[4], 2) == 0) {
                                int tick = atoi(wurds[5]);
                                printf("Rm'ing note\n");
                                minisynth_rm_note(ms, melody_num, tick);
                            }
                            else if (strncmp("madd", wurds[4], 4) == 0) {
                                int tick = 0;
                                int midi_note = 0;
                                sscanf(wurds[5], "%d:%d", &tick, &midi_note);
                                if (midi_note != 0) {
                                    printf("MAdding note\n");
                                    minisynth_add_micro_note(ms, melody_num,
                                                             tick, midi_note);
                                }
                            }
                            else if (strncmp("melody", wurds[4], 6) == 0) {
                                minisynth_reset_melody(ms, melody_num);
                                char_melody_to_midi_melody(ms, melody_num,
                                                           wurds, 5, num_wurds);
                            }
                            else if (strncmp("mmv", wurds[4], 2) == 0) {
                                int fromtick = atoi(wurds[5]);
                                int totick = atoi(wurds[6]);
                                printf("MMV'ing note\n");
                                minisynth_mv_micro_note(ms, melody_num,
                                                        fromtick, totick);
                            }
                            else if (strncmp("mrm", wurds[4], 3) == 0) {
                                int tick = atoi(wurds[5]);
                                printf("Rm'ing note\n");
                                minisynth_rm_micro_note(ms, melody_num, tick);
                            }
                        }
                    }
                    else if (strncmp("cp", wurds[2], 2) == 0) {
                        int pattern_num = atoi(wurds[3]);
                        int sg2 = 0;
                        int pattern_num2 = 0;
                        sscanf(wurds[4], "%d:%d", &sg2, &pattern_num2);
                        if (mixer_is_valid_soundgen_num(mixr, sg2) &&
                            mixr->sound_generators[sg2]->type == SYNTH_TYPE) {
                            minisynth *ms2 =
                                (minisynth *)mixr->sound_generators[sg2];
                            if (is_valid_melody_num(ms, pattern_num) &&
                                is_valid_melody_num(ms2, pattern_num2)) {

                                printf("Copying SYNTH pattern from %d:%d to "
                                       "%d:%d!\n",
                                       soundgen_num, pattern_num, sg2,
                                       pattern_num2);

                                midi_event **melody =
                                    minisynth_copy_midi_loop(ms, pattern_num);

                                minisynth_replace_midi_loop(ms2, melody,
                                                            pattern_num2);
                            }
                        }
                    }
                    else if (strncmp("delete", wurds[2], 3) == 0) {
                        if (strncmp("melody", wurds[3], 6) == 0) {
                            // minisynth_delete_melody(ms); // TODO implement
                            printf("Imagine i deleted your melody..\n");
                        }
                        else {
                            int melody = atoi(wurds[3]);
                            if (is_valid_melody_num(ms, melody)) {
                                printf("MELODY DELETE EVENT!\n");
                            }
                            int tick = atoi(wurds[4]);
                            if (tick < PPNS)
                                minisynth_rm_micro_note(ms, melody, tick);
                        }
                    }
                    else if (strncmp("dupe", wurds[2], 4) == 0) {
                        int pattern_num = atoi(wurds[3]);
                        int new_pattern_num = minisynth_add_melody(ms);
                        minisynth_dupe_melody(ms->melodies[pattern_num],
                                              ms->melodies[new_pattern_num]);
                    }
                    else if (strncmp("import", wurds[2], 6) == 0) {
                        printf("Importing file\n");
                        minisynth_import_midi_from_file(ms, wurds[3]);
                    }
                    else if (strncmp("keys", wurds[2], 4) == 0) {
                        keys(soundgen_num);
                    }
                    else if (strncmp("load", wurds[2], 4) == 0) {
                        char preset_name[20];
                        strncpy(preset_name, wurds[3], 19);
                        minisynth_load_settings(ms, preset_name);
                    }
                    else if (strncmp("midi", wurds[2], 4) == 0) {
                        mixr->midi_control_destination = SYNTH;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else if (strncmp("morph", wurds[2], 5) == 0) {
                        if (strncmp("every", wurds[3], 5) == 0) {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0) {
                                minisynth_set_morph_mode(ms, true);
                                ms->morph_every_n_loops = num_gens;
                            }
                            else {
                                printf("Need a number for every 'n'\n");
                            }
                        }
                        else if (strncmp("for", wurds[3], 3) == 0) {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0) {
                                minisynth_set_morph_mode(ms, true);
                                ms->max_generation = num_gens;
                            }
                            else {
                                printf("Need a number for 'for'\n");
                            }
                        }
                        else { // just toggle
                            minisynth_set_morph_mode(ms, 1 - ms->morph_mode);
                        }

                        printf("Synth morph mode : %s\n",
                               ms->morph_mode ? "true" : "false");
                    }
                    else if (strncmp("multi", wurds[2], 5) == 0) {
                        if (strncmp("true", wurds[3], 4) == 0) {
                            minisynth_set_multi_melody_mode(ms, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0) {
                            minisynth_set_multi_melody_mode(ms, false);
                        }
                        printf("Synth multi mode : %s\n",
                               ms->multi_melody_mode ? "true" : "false");
                    }
                    else if (strncmp("nudge", wurds[2], 5) == 0) {
                        int sixteenth = atoi(wurds[3]);
                        if (sixteenth < 16) {
                            printf("Nudging Melody along %d sixteenthzzzz!\n",
                                   sixteenth);
                            minisynth_nudge_melody(ms, ms->cur_melody,
                                                   sixteenth);
                        }
                    }
                    else if (strncmp("print", wurds[2], 5) == 0) {
                        minisynth_print(ms);
                    }
                    else if (strncmp("quantize", wurds[2], 8) == 0) {
                        int melody_num = atoi(wurds[3]);
                        if (is_valid_melody_num(ms, melody_num)) {
                            printf("QuantiZe!\n");
                            midi_event **melody = ms->melodies[ms->cur_melody];
                            midi_melody_quantize(melody);
                        }
                    }
                    else if (strncmp("rand", wurds[2], 4) == 0) {
                        minisynth_rand_settings(ms);
                    }
                    else if (strncmp("reset", wurds[2], 5) == 0) {
                        if (strncmp("all", wurds[3], 3) == 0) {
                            minisynth_reset_melody_all(ms);
                        }
                        else {
                            int melody_num = atoi(wurds[3]);
                            minisynth_reset_melody(ms, melody_num);
                        }
                    }
                    else if (strncmp("save", wurds[2], 4) == 0) {
                        char preset_name[20];
                        strncpy(preset_name, wurds[3], 19);
                        minisynth_save_settings(ms, preset_name);
                    }
                    else if (strncmp("switch", wurds[2], 6) == 0) {
                        int melody_num = atoi(wurds[3]);
                        minisynth_switch_melody(ms, melody_num);
                    }
                    else if (strncmp("sustain", wurds[2], 5) == 0) {
                        if (strncmp("true", wurds[3], 4) == 0) {
                            minisynth_set_sustain_override(ms, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0) {
                            minisynth_set_sustain_override(ms, false);
                        }
                        printf("Synth Sustain Override : %s\n",
                               ms->m_settings.m_sustain_override ? "true"
                                                                 : "false");
                        for (int i = 0; i < MAX_VOICES; i++) {
                            if (ms->m_voices[i]) {
                                printf("EG sustain: %d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg1.m_sustain_override);
                                printf("EG sustain: %d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg2.m_sustain_override);
                                printf("EG sustain: %d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg3.m_sustain_override);
                                printf("EG sustain: %d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg4.m_sustain_override);
                            }
                        }
                    }
                }
            }
        }

        // CHAOS COMMANDS
        else if (strncmp("chaos", wurds[0], 6) == 0) {

            if (strncmp("monkey", wurds[1], 6) == 0) {
                int soundgen_num = atoi(wurds[2]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                    add_chaosmonkey(soundgen_num);
                }
            }
            else {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
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
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                int delay_len_ms = atoi(wurds[2]);
                add_delay_soundgen(mixr->sound_generators[soundgen_num],
                                   delay_len_ms);
            }
        }
        else if (strncmp("moddelay", wurds[0], 7) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                add_moddelay_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("modfilter", wurds[0], 9) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                add_modfilter_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("reverb", wurds[0], 6) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                add_reverb_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("distort", wurds[0], 7) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                add_distortion_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("decimate", wurds[0], 8) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                add_decimator_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("env", wurds[0], 3) == 0) {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
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
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
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
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
                int nbeats = atoi(wurds[2]);
                int sixteenth = atoi(wurds[3]);
                add_beatrepeat_soundgen(mixr->sound_generators[soundgen_num],
                                        nbeats, sixteenth);
            }
        }

        else if (strncmp("fx", wurds[0], 2) == 0) {
            int soundgen_num = atoi(wurds[1]);
            int fx_num = atoi(wurds[2]);
            if (is_valid_fx_num(soundgen_num, fx_num)) {
                fx *f = mixr->sound_generators[soundgen_num]->effects[fx_num];
                if (f->type == DELAY) {
                    // printf("Changing Dulay!\n");
                    stereodelay *sd = (stereodelay *)f;
                    double val = atof(wurds[4]);
                    // keep these strings in sync with status() output
                    if (strncmp("delayms", wurds[3], 7) == 0)
                        stereo_delay_set_delay_time_ms(sd, val);
                    else if (strncmp("fb", wurds[3], 2) == 0)
                        stereo_delay_set_feedback_percent(sd, val);
                    else if (strncmp("ratio", wurds[3], 5) == 0)
                        stereo_delay_set_delay_ratio(sd, val);
                    else if (strncmp("wetmx", wurds[3], 5) == 0)
                        stereo_delay_set_wet_mix(sd, val);
                    else if (strncmp("mode", wurds[3], 4) == 0)
                        stereo_delay_set_mode(sd, val);
                    else
                        printf("<bleurgh!>\n");
                }
                else if (f->type == REVERB) {
                    // printf("Re-re-re----verb!\n");
                    reverb *r = (reverb *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("predelayms", wurds[3], 10) == 0)
                        reverb_set_pre_delay_msec(r, val);
                    else if (strncmp("predelayattDb", wurds[3], 13) == 0)
                        reverb_set_pre_delay_atten_db(r, val);
                    else if (strncmp("inputLPFg", wurds[3], 9) == 0)
                        reverb_set_input_lpf_g(r, val);
                    else if (strncmp("lpf2g2", wurds[3], 6) == 0)
                        reverb_set_lpf2_g2(r, val);
                    else if (strncmp("reverbtime", wurds[3], 10) == 0)
                        reverb_set_rt60(r, val);
                    else if (strncmp("wetmx", wurds[3], 5) == 0)
                        reverb_set_wet_pct(r, val);
                    else if (strncmp("APF1delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 1, val);
                    else if (strncmp("APF1g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 1, val);
                    else if (strncmp("APF2delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 2, val);
                    else if (strncmp("APF2g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 2, val);
                    else if (strncmp("APF3delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 3, val);
                    else if (strncmp("APF3g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 3, val);
                    else if (strncmp("APF4delayms", wurds[3], 11) == 0)
                        reverb_set_apf_delay_msec(r, 4, val);
                    else if (strncmp("APF4g", wurds[3], 5) == 0)
                        reverb_set_apf_g(r, 4, val);
                    else if (strncmp("comb1delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 1, val);
                    else if (strncmp("comb2delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 2, val);
                    else if (strncmp("comb3delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 3, val);
                    else if (strncmp("comb4delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 4, val);
                    else if (strncmp("comb5delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 5, val);
                    else if (strncmp("comb6delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 6, val);
                    else if (strncmp("comb7delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 7, val);
                    else if (strncmp("comb8delayms", wurds[3], 12) == 0)
                        reverb_set_comb_delay_msec(r, 8, val);
                }
                else if (f->type == BEATREPEAT) {
                    beatrepeat *br = (beatrepeat *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("numbeats", wurds[3], 8) == 0)
                        beatrepeat_change_num_beats_to_repeat(br, val);
                    else if (strncmp("sixteenth", wurds[3], 9) == 0)
                        beatrepeat_change_selected_sixteenth(br, val);
                }
                else if (f->type == MODDELAY) {
                    mod_delay *md = (mod_delay *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("depth", wurds[3], 5) == 0) {
                        mod_delay_set_depth(md, val);
                    }
                    else if (strncmp("rate", wurds[3], 4) == 0) {
                        mod_delay_set_rate(md, val);
                    }
                    else if (strncmp("fb", wurds[3], 8) == 0) {
                        mod_delay_set_feedback_percent(md, val);
                    }
                    else if (strncmp("offset", wurds[3], 12) == 0) {
                        mod_delay_set_chorus_offset(md, val);
                    }
                    else if (strncmp("type", wurds[3], 7) == 0) {
                        mod_delay_set_mod_type(md, (unsigned int)val);
                    }
                    else if (strncmp("lfo", wurds[3], 7) == 0) {
                        mod_delay_set_lfo_type(md, (unsigned int)val);
                    }
                }
                else if (f->type == MODFILTER) {
                    modfilter *mf = (modfilter *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("depthfc", wurds[3], 7) == 0) {
                        modfilter_set_mod_depth_fc(mf, val);
                    }
                    else if (strncmp("ratefc", wurds[3], 6) == 0) {
                        modfilter_set_mod_rate_fc(mf, val);
                    }
                    else if (strncmp("depthq", wurds[3], 6) == 0) {
                        modfilter_set_mod_depth_q(mf, val);
                    }
                    else if (strncmp("rateq", wurds[3], 5) == 0) {
                        modfilter_set_mod_rate_q(mf, val);
                    }
                    else if (strncmp("LFOphase", wurds[3], 8) == 0) {
                        modfilter_set_lfo_waveform(mf, val);
                    }
                    else if (strncmp("LFO", wurds[3], 3) == 0) {
                        modfilter_set_lfo_phase(mf, val);
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
    char const *sep = " ";
    char *tok, *last_s;
    for (tok = strtok_r(line, sep, &last_s); tok;
         tok = strtok_r(NULL, sep, &last_s)) {
        strncpy(wurds[num_wurds++], tok, SIZE_OF_WURD);
        if (num_wurds == NUM_WURDS)
            break;
    }
    return num_wurds;
}

void char_array_to_string_sequence(sequencer *seq, char *dest_pattern,
                                   char char_array[NUM_WURDS][SIZE_OF_WURD],
                                   int start, int end)
{
    for (int i = start; i < end; i++) {
        strcat(dest_pattern, char_array[i]);
        if (i != (end - 1))
            strcat(dest_pattern, " ");
    }
}

void char_array_to_seq_string_pattern(sequencer *seq, char *dest_pattern,
                                      char char_array[NUM_WURDS][SIZE_OF_WURD],
                                      int start, int end)
{
    if (strncmp("all24", char_array[start], 5) == 0) {
        if (seq->pattern_len == 16) {
            seq_set_gridsteps(seq, 24);
        }
        strncat(dest_pattern,
                "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23",
                151);
    }
    else if (strncmp("all", char_array[start], 3) == 0) {
        if (seq->pattern_len == 16) {
            strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", 127);
        }
        else if (seq->pattern_len == 24) {
            strncat(
                dest_pattern,
                "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23",
                151);
        }
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

bool is_valid_sample_num(looper *s, int sample_num)
{
    if (sample_num < s->num_samples) {
        return true;
    }
    return false;
}

bool is_valid_fx_num(int soundgen_num, int fx_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
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
    printf(
        COOL_COLOR_GREEN
        "\nBeat it, ya val jerk...\n" ANSI_COLOR_RESET); // Thrashin' reference
    pa_teardown();
    exit(0);
}

void parse_sequencer_command(sequencer *seq, char wurds[][SIZE_OF_WURD],
                             int num_wurds, char *pattern)
{
    if (strncmp("add", wurds[2], 3) == 0) {
        printf("Adding\n");
        char_array_to_seq_string_pattern(seq, pattern, wurds, 3, num_wurds);
        add_char_pattern(seq, pattern);
    }
    else if (strncmp("grid", wurds[2], 4) == 0) {
        int gridsteps = atoi(wurds[3]);
        if (gridsteps != 16 && gridsteps != 24) {
            printf("Gridsteps must be either 16 or 24 (not %d)\n", gridsteps);
            return;
        }
        printf("Change gridsteps to %d\n", gridsteps);
        seq_set_gridsteps(seq, gridsteps);
    }
    else if (strncmp("print", wurds[2], 5) == 0) {
        int pattern_num = atoi(wurds[3]);
        if (seq_is_valid_pattern_num(seq, pattern_num)) {
            printf("Printing pattern for %d\n", pattern_num);
            seq_print_pattern(seq, pattern_num);
        }
    }
    else if (strncmp("randamp", wurds[2], 6) == 0) {
        seq_set_randamp(seq, 1 - seq->randamp_on);
        printf("Toggling randamp to %s \n", seq->randamp_on ? "true" : "false");
    }
    else if (strncmp("multi", wurds[2], 5) == 0) {
        if (strncmp("true", wurds[3], 4) == 0) {
            seq_set_multi_pattern_mode(seq, true);
        }
        else if (strncmp("false", wurds[3], 5) == 0) {
            seq_set_multi_pattern_mode(seq, false);
        }
        printf("Sequencer multi mode : %s\n",
               seq->multi_pattern_mode ? "true" : "false");
    }
    else if (strncmp("change", wurds[2], 6) == 0) {
        if (strncmp("markov", wurds[3], 6) == 0) {
            printf("MARKOV!\n");
            if (strncmp("haus", wurds[4], 4) == 0) {
                printf("HAUS!\n");
                seq_set_markov_mode(seq, MARKOVHAUS);
            }
            else if (strncmp("boombap", wurds[4], 7) == 0) {
                printf("BOOMBAP!\n");
                seq_set_markov_mode(seq, MARKOVBOOMBAP);
            }
        }
        if (strncmp("bitwise", wurds[3], 6) == 0) {
            printf("BITWISE CHANGE!!\n");
            int bitwise_mode = atoi(wurds[4]);
            seq_set_bitwise_mode(seq, bitwise_mode);
        }
        else {
            int pattern_num = atoi(wurds[3]);
            if (seq_is_valid_pattern_num(seq, pattern_num)) {
                if (strncmp("add", wurds[4], 3) == 0) {
                    int hit = atoi(wurds[5]);
                    printf("Adding a hit to %d\n", hit);
                    seq_add_hit(seq, pattern_num, hit);
                }
                else if (strncmp("madd", wurds[4], 3) == 0) { // midi pulses
                    int hit = atoi(wurds[5]);
                    printf("Adding a hit to %d\n", hit);
                    seq_add_micro_hit(seq, pattern_num, hit);
                }
                else if (strncmp("amp", wurds[4], 3) == 0) {
                    char_array_to_seq_string_pattern(seq, pattern, wurds, 5,
                                                     num_wurds);
                    printf("Setting pattern AMP to %s\n", pattern);
                    seq_set_sample_amp_from_char_pattern(seq, pattern_num,
                                                         pattern);
                }
                else if (strncmp("mv", wurds[4], 2) ==
                         0) { // deals in 16th or 24th
                    int hitfrom = atoi(wurds[5]);
                    int hitto = atoi(wurds[6]);
                    seq_mv_hit(seq, pattern_num, hitfrom, hitto);
                }
                else if (strncmp("mmv", wurds[4], 2) ==
                         0) { // deals in midi pulses
                    int hitfrom = atoi(wurds[5]);
                    int hitto = atoi(wurds[6]);
                    seq_mv_micro_hit(seq, pattern_num, hitfrom, hitto);
                }
                else if (strncmp("numloops", wurds[4], 8) == 0) {
                    int numloops = atoi(wurds[5]);
                    if (numloops != 0) {
                        seq_change_num_loops(seq, pattern_num, numloops);
                    }
                }
                else if (strncmp("pattern", wurds[4], 7) == 0) {
                    char_array_to_seq_string_pattern(seq, pattern, wurds, 5,
                                                     num_wurds);
                    printf("Changing pattern to %s\n", pattern);
                    change_char_pattern(seq, pattern_num, pattern);
                }
                else if (strncmp("rm", wurds[4], 2) == 0) {
                    int hit = atoi(wurds[5]);
                    printf("Rm'ing hit to %d\n", hit);
                    seq_rm_hit(seq, pattern_num, hit);
                }
                else if (strncmp("mrm", wurds[4], 2) == 0) {
                    int hit = atoi(wurds[5]);
                    printf("Rm'ing hit to %d\n", hit);
                    seq_rm_micro_hit(seq, pattern_num, hit);
                }
                else if (strncmp("swing", wurds[4], 5) == 0) {
                    int swing_setting = atoi(wurds[5]);
                    printf("changing swing to %d for pattern num %d\n",
                           swing_setting, pattern_num);
                    seq_swing_pattern(seq, pattern_num, swing_setting);
                }
            }
        }
    }
    else if (strncmp("life", wurds[2], 4) == 0) {
        if (strncmp("every", wurds[3], 5) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_game_of_life(seq, true);
                seq->life_every_n_loops = num_gens;
            }
            else {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_game_of_life(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else {
                printf("Need a number for 'for'\n");
            }
        }
        else {
            seq_set_game_of_life(seq, 1 - seq->game_of_life_on);
        }
    }
    else if (strncmp("markov", wurds[2], 4) == 0) {
        if (strncmp("every", wurds[3], 5) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_markov(seq, true);
                seq->markov_every_n_loops = num_gens;
            }
            else {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_markov(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else {
                printf("Need a number for 'for'\n");
            }
        }
        else {
            seq_set_markov(seq, 1 - seq->markov_on);
        }
    }
    else if (strncmp("bitwise", wurds[2], 4) == 0) {
        if (strncmp("every", wurds[3], 5) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_bitwise(seq, true);
                seq->bitwise_every_n_loops = num_gens;
            }
            else {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_bitwise(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else {
                printf("Need a number for 'for'\n");
            }
        }
        else {
            seq_set_bitwise(seq, 1 - seq->game_of_life_on);
        }
    }
    else if (strncmp("euclid", wurds[2], 6) == 0) {
        printf("Euclidean, yo!\n");
        if (strncmp("every", wurds[3], 5) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_euclidean(seq, true);
                seq->euclidean_every_n_loops = num_gens;
            }
            else {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0) {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0) {
                seq_set_euclidean(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else {
                printf("Need a number for 'for'\n");
            }
        }
        else {
            seq_set_euclidean(seq, 1 - seq->game_of_life_on);
        }
    }
}

bool parse_minisynth_settings_change(minisynth *ms, char wurds[][SIZE_OF_WURD],
                                     int num_wurds)
{
    if (strncmp("attackms", wurds[3], 8) == 0) {
        printf("Minisynth change Attack Time Ms!\n");
        double val = atof(wurds[4]);
        minisynth_set_attack_time_ms(ms, val);
        return true;
    }
    else if (strncmp("decayms", wurds[3], 7) == 0) {
        printf("Minisynth change Decay/Relase Time MS!\n");
        double val = atof(wurds[4]);
        minisynth_set_decay_time_ms(ms, val);
        return true;
    }
    else if (strncmp("delayfb", wurds[3], 7) == 0) {
        printf("Minisynth change Delay Feedback!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_feedback_pct(ms, val);
        return true;
    }
    else if (strncmp("delayr", wurds[3], 6) == 0) {
        printf("Minisynth change Delay Ratio!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_ratio(ms, val);
        return true;
    }
    else if (strncmp("delaymode", wurds[3], 9) == 0) {
        printf("Minisynth change DELAY MODE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_delay_mode(ms, val);
        return true;
    }
    else if (strncmp("delayms", wurds[3], 7) == 0) {
        printf("Minisynth change Delay Time Ms!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_time_ms(ms, val);
        return true;
    }
    else if (strncmp("delaymx", wurds[3], 7) == 0) {
        printf("Minisynth change Delay Wet Mix!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_wetmix(ms, val);
        return true;
    }
    else if (strncmp("detune", wurds[3], 6) == 0) {
        printf("Minisynth change DETUNE!\n");
        double val = atof(wurds[4]);
        minisynth_set_detune(ms, val);
        return true;
    }
    else if (strncmp("eg1dcaint", wurds[3], 9) == 0) {
        printf("Minisynth change EG1 DCA Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_eg1_dca_int(ms, val);
        return true;
    }
    else if (strncmp("eg1filterint", wurds[3], 12) == 0) {
        printf("Minisynth change EG1 Filter Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_eg1_filter_int(ms, val);
        return true;
    }
    else if (strncmp("eg1oscint", wurds[3], 9) == 0) {
        printf("Minisynth change EG1 Osc Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_eg1_osc_int(ms, val);
        return true;
    }
    else if (strncmp("fc", wurds[3], 2) == 0) {
        printf("Minisynth change Filter Cutoff!\n");
        double val = atof(wurds[4]);
        minisynth_set_filter_fc(ms, val);
        return true;
    }
    else if (strncmp("fq", wurds[3], 2) == 0) {
        printf("Minisynth change Filter Qualivity!\n");
        double val = atof(wurds[4]);
        minisynth_set_filter_fq(ms, val);
        return true;
    }
    else if (strncmp("ktint", wurds[3], 5) == 0) {
        printf("Minisynth change Filter Keytrack Intensity!\n");
        double val = atof(wurds[4]);
        minisynth_set_keytrack_int(ms, val);
        return true;
    }
    else if (strncmp("kt", wurds[3], 2) == 0) {
        printf("Minisynth change Filter Keytrack!\n");
        int val = atoi(wurds[4]);
        minisynth_set_keytrack(ms, val);
        return true;
    }
    else if (strncmp("legato", wurds[3], 6) == 0) {
        printf("Minisynth change LEGATO!\n");
        int val = atoi(wurds[4]);
        minisynth_set_legato_mode(ms, val);
        return true;
    }
    else if (strncmp("lfo1ampint", wurds[3], 10) == 0) {
        printf("Minisynth change LFO1 Amp Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo1_amp_int(ms, val);
        return true;
    }
    else if (strncmp("lfo1amp", wurds[3], 7) == 0) {
        printf("Minisynth change LFO1 AMP!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo1_amp(ms, val);
        return true;
    }
    else if (strncmp("lfo1filterint", wurds[3], 13) == 0) {
        printf("Minisynth change LFO1 Filter FC Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo1_filter_fc_int(ms, val);
        return true;
    }
    else if (strncmp("lfo1rate", wurds[3], 8) == 0) {
        printf("Minisynth change LFO1 rate!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo1_rate(ms, val);
        return true;
    }
    else if (strncmp("lfo1panint", wurds[3], 10) == 0) {
        printf("Minisynth change LFO1 Pan Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo1_pan_int(ms, val);
        return true;
    }
    else if (strncmp("lfo1pitch", wurds[3], 9) == 0) {
        printf("Minisynth change LFO1 Pitch!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo1_pitch(ms, val);
        return true;
    }
    else if (strncmp("lfowave", wurds[3], 7) == 0) {
        printf("Minisynth change LFO1 Wave!\n");
        int val = atoi(wurds[4]);
        minisynth_set_lfo1_wave(ms, val);
        return true;
    }
    else if (strncmp("ndscale", wurds[3], 7) == 0) {
        printf("Minisynth change Note Number to Decay Scaling!\n");
        int val = atoi(wurds[4]);
        minisynth_set_note_to_decay_scaling(ms, val);
        return true;
    }
    else if (strncmp("noisedb", wurds[3], 7) == 0) {
        printf("Minisynth change Noise Osc DB!\n");
        double val = atof(wurds[4]);
        minisynth_set_noise_osc_db(ms, val);
        return true;
    }
    else if (strncmp("oct", wurds[3], 3) == 0) {
        printf("Minisynth change OCTAVE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_octave(ms, val);
        return true;
    }
    else if (strncmp("pitchrange", wurds[3], 10) == 0) {
        printf("Minisynth change Pitchbend RANGE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_pitchbend_range(ms, val);
        return true;
    }
    else if (strncmp("porta", wurds[3], 5) == 0) {
        printf("Minisynth change PORTAMENTO Time!\n");
        double val = atof(wurds[4]);
        minisynth_set_portamento_time_ms(ms, val);
        return true;
    }
    else if (strncmp("pw", wurds[3], 2) == 0) {
        printf("Minisynth change PULSEWIDTH Pct!\n");
        double val = atof(wurds[4]);
        minisynth_set_pulsewidth_pct(ms, val);
        return true;
    }
    else if (strncmp("subosc", wurds[3], 6) == 0) {
        printf("Minisynth change SubOSC DB!\n");
        double val = atof(wurds[4]);
        minisynth_set_sub_osc_db(ms, val);
        return true;
    }
    else if (strncmp("sustainlvl", wurds[3], 10) == 0) {
        double val = atof(wurds[4]);
        printf("Minisynth change Sustain Level! %.2f\n", val);
        minisynth_set_sustain(ms, val);
        return true;
    }
    else if (strncmp("sustainms", wurds[3], 9) == 0) {
        printf("Minisynth change Sustain Time ms!\n");
        double val = atof(wurds[4]);
        minisynth_set_sustain_time_ms(ms, val);
        return true;
    }
    else if (strncmp("sustain16th", wurds[3], 11) == 0) {
        printf("Minisynth change Sustain Time 16th!\n");
        double val = atof(wurds[4]);
        minisynth_set_sustain_time_sixteenth(ms, val);
        return true;
    }
    else if (strncmp("sustain", wurds[3], 7) == 0) {
        printf("Minisynth change SUSTAIN OVERRIDE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_sustain_override(ms, val);
        return true;
    }
    else if (strncmp("vascale", wurds[3], 7) == 0) {
        printf("Minisynth change Velocity to Attack Scaling!\n");
        int val = atoi(wurds[4]);
        minisynth_set_velocity_to_attack_scaling(ms, val);
        return true;
    }
    else if (strncmp("voice", wurds[3], 5) == 0) {
        printf("Minisynth change VOICE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_voice_mode(ms, val);
        return true;
    }
    else if (strncmp("vol", wurds[3], 3) == 0) {
        printf("Minisynth change VOLUME!\n");
        double val = atof(wurds[4]);
        minisynth_set_vol(ms, val);
        return true;
    }
    else if (strncmp("zero", wurds[3], 4) == 0) {
        printf("Minisynth change REST-To-ZERO!\n");
        int val = atoi(wurds[4]);
        minisynth_set_reset_to_zero(ms, val);
        return true;
    }
    return false;
}

void char_melody_to_midi_melody(minisynth *ms, int dest_melody,
                                char char_array[NUM_WURDS][SIZE_OF_WURD],
                                int start, int end)
{
    for (int i = start; i < end; i++) {
        int tick = 0;
        int midi_note = 0;
        sscanf(char_array[i], "%d:%d", &tick, &midi_note);
        if (midi_note != 0) {
            printf("Adding %d:%d\n", tick, midi_note);
            minisynth_add_note(ms, dest_melody, tick, midi_note);
        }
    }
}
