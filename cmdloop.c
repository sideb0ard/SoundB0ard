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
#include "basicfilterpass.h"
#include "beatrepeat.h"
#include "chaosmonkey.h"
#include "cmdloop.h"
#include "defjams.h"
#include "digisynth.h"
#include "distortion.h"
#include "dynamics_processor.h"
#include "envelope.h"
#include "envelope_follower.h"
#include "granulator.h"
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
#include "synthbase.h"
#include "synthdrum_sequencer.h"
#include "table.h"
#include "utils.h"
#include "waveshaper.h"

extern mixer *mixr;
extern char *key_names[NUM_KEYS];

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
    while ((line = readline(prompt)) != NULL)
    {
        if (line[0] != 0)
        {
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
         cmd = strtok_r(NULL, sep, &last_s))
    {

        char tmp[128];
        strncpy((char *)tmp, cmd, 127);

        int num_wurds = parse_wurds_from_cmd(wurds, tmp);

        // Understanding these commands should make more sense when viewed
        // against the help output, which details each command
        // (TODO) some kid of doxygen-like scheme to write help output here

        //////  MIXER COMMANDS  /////////////////////////
        if (strncmp("help", wurds[0], 4) == 0)
        {
            print_help();
        }

        else if (strncmp("quit", wurds[0], 4) == 0 ||
                 strncmp("exit", wurds[0], 4) == 0)
        {
            exxit();
        }

        else if (strncmp("bpm", wurds[0], 3) == 0)
        {
            int bpm = atoi(wurds[1]);
            if (bpm > 0)
                mixer_update_bpm(mixr, bpm);
        }

        else if (strncmp("keys", wurds[0], 4) == 0)
        {
            for (int i = 0; i < NUM_KEYS; i++)
            {
                char *key = key_names[i];
                printf("%d [%s] ", i, key);
            }
            printf("\n");
        }
        else if (strncmp("key", wurds[0], 3) == 0)
        {
            int key = atoi(wurds[1]);
            if (key >= 0 && key < NUM_KEYS)
            {
                printf("Changing KEY!\n");
                mixr->key = key;
            }
        }
        else if (strncmp("new", wurds[0], 3) == 0)
        {
            if (strncmp("spork", wurds[1], 5) == 0)
            {
                printf("Sp0rky!\n");
                double freq = atof(wurds[2]);
                if (freq > 0.)
                {
                    mixer_add_spork(mixr, freq);
                }
                else
                {
                    mixer_add_spork(mixr, 440);
                }
            }
        }

        else if (strncmp("preview", wurds[0], 7) == 0)
        {
            if (is_valid_file(wurds[1]))
            {
                mixer_preview_track(mixr, wurds[1]);
            }
        }

        else if (strncmp("debug", wurds[0], 5) == 0)
        {
            if (strncmp("on", wurds[1], 2) == 0 ||
                strncmp("true", wurds[1], 4) == 0)
            {
                printf("Enabling debug mode\n");
                mixr->debug_mode = true;
            }
            else if (strncmp("off", wurds[1], 2) == 0 ||
                     strncmp("false", wurds[1], 5) == 0)
            {
                printf("Disabling debug mode\n");
                mixr->debug_mode = false;
            }
        }

        else if (strncmp("ps", wurds[0], 2) == 0)
        {
            mixer_ps(mixr);
        }

        else if (strncmp("quiet", wurds[0], 5) == 0)
        {
            for (int i = 0; i < mixr->soundgen_num; i++)
                mixr->sound_generators[i]->setvol(mixr->sound_generators[i],
                                                  0.0);
        }
        else if (strncmp("ls", wurds[0], 2) == 0)
        {
            list_sample_dir(wurds[1]);
        }

        else if (strncmp("rm", wurds[0], 3) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                printf("Deleting SOUND GEN %d\n", soundgen_num);
                mixer_del_soundgen(mixr, soundgen_num);
            }
        }
        else if (strncmp("start", wurds[0], 5) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                printf("Starting SOUND GEN %d\n", soundgen_num);
                SOUNDGEN *sg = mixr->sound_generators[soundgen_num];
                sg->start(sg);
            }
        }
        else if (strncmp("stop", wurds[0], 5) == 0)
        {
            if (strncmp(wurds[1], "all", 3) == 0)
            {
                for (int i = 0; i < mixr->soundgen_num; i++)
                {
                    SOUNDGEN *sg = mixr->sound_generators[i];
                    if (sg != NULL)
                        sg->stop(sg);
                }
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
                {
                    printf("Stopping SOUND GEN %d\n", soundgen_num);
                    SOUNDGEN *sg = mixr->sound_generators[soundgen_num];
                    sg->stop(sg);
                }
            }
        }
        else if (strncmp("down", wurds[0], 4) == 0 ||
                 strncmp("up", wurds[0], 3) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                SBMSG *msg = new_sbmsg();
                msg->sound_gen_num = soundgen_num;
                if (strcmp("up", wurds[0]) == 0)
                {
                    strncpy(msg->cmd, "fadeuprrr", 19);
                }
                else
                {
                    strncpy(msg->cmd, "fadedownrrr", 19);
                }
                thrunner(msg);
            }
        }

        else if (strncmp("vol", wurds[0], 3) == 0)
        {
            if (strncmp("mixer", wurds[1], 5) == 0)
            {
                double vol = atof(wurds[2]);
                mixer_vol_change(mixr, vol);
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
                {
                    double vol = atof(wurds[2]);
                    vol_change(mixr, soundgen_num, vol);
                }
            }
        }

        else if (strncmp("scene", wurds[0], 5) == 0)
        {
            if (strncmp("new", wurds[1], 4) == 0)
            {
                int num_bars = atoi(wurds[2]);
                if (num_bars == 0)
                    num_bars = 4; // default
                int new_scene_num = mixer_add_scene(mixr, num_bars);
                printf("New scene %d with %d bars!\n", new_scene_num, num_bars);
            }
            else if (strncmp("mode", wurds[1], 4) == 0)
            {
                if (strncmp("on", wurds[2], 2) == 0)
                {
                    mixr->scene_mode = true;
                    mixr->scene_start_pending = true;
                }
                else if (strncmp("off", wurds[2], 3) == 0)
                {
                    mixr->scene_mode = false;
                    mixr->scene_start_pending = false;
                }
                else
                    mixr->scene_mode = 1 - mixr->scene_mode;
                printf("Mode scene! %s\n", mixr->scene_mode ? "true" : "false");
            }
            else
            {
                int scene_num = atoi(wurds[1]);
                if (mixer_is_valid_scene_num(mixr, scene_num))
                {
                    printf("Changing scene %d\n", scene_num);
                    if (strncmp("add", wurds[2], 3) == 0)
                    {
                        int sg_num = 0;
                        int sg_track_num = 0;
                        sscanf(wurds[3], "%d:%d", &sg_num, &sg_track_num);
                        if (mixer_is_valid_soundgen_track_num(mixr, sg_num,
                                                              sg_track_num))
                        {
                            printf("Adding sg %d %d\n", sg_num, sg_track_num);
                            mixer_add_soundgen_track_to_scene(
                                mixr, scene_num, sg_num, sg_track_num);
                        }
                    }
                    else if (strncmp("cp", wurds[2], 2) == 0)
                    {
                        int scene_num2 = atoi(wurds[3]);
                        if (mixer_is_valid_scene_num(mixr, scene_num2))
                        {
                            printf("Copying scene %d "
                                   "to %d\n",
                                   scene_num, scene_num2);
                            mixer_cp_scene(mixr, scene_num, scene_num2);
                        }
                        else
                        {
                            printf("Not copying scene %d "
                                   "-- %d is not a valid "
                                   "destination\n",
                                   scene_num, scene_num2);
                        }
                    }
                    else if (strncmp("dupe", wurds[2], 4) == 0)
                    {
                        printf("Duplicating scene %d\n", scene_num);
                        int default_num_bars = 4;
                        int new_scene_num =
                            mixer_add_scene(mixr, default_num_bars);
                        mixer_cp_scene(mixr, scene_num, new_scene_num);
                    }
                    else if (strncmp("rm", wurds[2], 2) == 0)
                    {
                        int sg_num = 0;
                        int sg_track_num = 0;
                        sscanf(wurds[3], "%d:%d", &sg_num, &sg_track_num);
                        if (mixer_is_valid_soundgen_track_num(mixr, sg_num,
                                                              sg_track_num))
                        {
                            printf("Removing sg %d %d\n", sg_num, sg_track_num);
                            mixer_rm_soundgen_track_from_scene(
                                mixr, scene_num, sg_num, sg_track_num);
                        }
                    }
                    else
                    {
                        printf("Queueing scene %d\n", scene_num);
                        mixr->scene_start_pending = true;
                        mixr->current_scene = scene_num;
                    }
                }
            }
        }

        else if (strncmp("synthdrum", wurds[0], 9) == 0 ||
                 strncmp("sd", wurds[0], 2) == 0)
        {
            char pattern[151] = {0};
            if (strncmp("new", wurds[1], 4) == 0)
            {
                synthdrum_sequencer *sds = new_synthdrum_seq();
                char_array_to_seq_string_pattern(&sds->m_seq, pattern, wurds, 2,
                                                 num_wurds);
                int sgnum = add_sound_generator(
                    mixr,
                    (SOUNDGEN *)sds); //  add_seq_char_pattern(mixr,
                                      //  wurds[1], pattern);
                printf("New Sound Generator (SD): %d\n", sgnum);
                pattern_char_to_pattern(
                    &sds->m_seq, pattern,
                    sds->m_seq.patterns[sds->m_seq.num_patterns++]);
            }
            if (strncmp("list", wurds[1], 4) == 0 ||
                strncmp("ls", wurds[1], 2) == 0)
            {
                printf("Listing SYNTHDRUM patches.. \n");
                synthdrum_list_patches();
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        SYNTHDRUM_TYPE)
                {

                    synthdrum_sequencer *sds =
                        (synthdrum_sequencer *)
                            mixr->sound_generators[soundgen_num];

                    if (strncmp("osc1_wav", wurds[2], 8) == 0)
                    {
                        int val = atoi(wurds[3]);
                        synthdrum_set_osc_wav(sds, 1, val);
                    }
                    else if (strncmp("osc1_fo", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_fo(sds, 1, val);
                    }
                    else if (strncmp("osc1_amp", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_amp(sds, 1, val);
                    }
                    else if (strncmp("eg1_attack", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_attack(sds, 1, val);
                    }
                    else if (strncmp("eg1_decay", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_decay(sds, 1, val);
                    }
                    else if (strncmp("eg1_sustain_ms", wurds[2], 14) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_sustain_ms(sds, 1, val);
                    }
                    else if (strncmp("eg1_sustain_level", wurds[2], 17) == 0)
                    {
                        printf("Sustain LEvel!\n");
                        double val = atof(wurds[3]);
                        eg_set_sustain_level(&sds->m_eg1, val);
                    }
                    else if (strncmp("eg1_release", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_release(sds, 1, val);
                    }
                    else if (strncmp("osc2_wav", wurds[2], 8) == 0)
                    {
                        int val = atoi(wurds[3]);
                        synthdrum_set_osc_wav(sds, 2, val);
                    }
                    else if (strncmp("osc2_fo", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_fo(sds, 2, val);
                    }
                    else if (strncmp("osc2_amp", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_amp(sds, 2, val);
                    }
                    else if (strncmp("osc3_wav", wurds[2], 8) == 0)
                    {
                        int val = atoi(wurds[3]);
                        synthdrum_set_osc_wav(sds, 3, val);
                    }
                    else if (strncmp("osc3_fo", wurds[2], 7) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_osc_fo(sds, 3, val);
                    }
                    else if (strncmp("eg2_attack", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_attack(sds, 2, val);
                    }
                    else if (strncmp("eg2_decay", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_decay(sds, 2, val);
                    }
                    else if (strncmp("eg2_sustain_ms", wurds[2], 14) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_sustain_ms(sds, 2, val);
                    }
                    else if (strncmp("eg2_sustain_level", wurds[2], 17) == 0)
                    {
                        printf("Sustain LEvel!\n");
                        double val = atof(wurds[3]);
                        eg_set_sustain_level(&sds->m_eg2, val);
                    }
                    else if (strncmp("eg2_release", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_release(sds, 2, val);
                    }
                    else if (strncmp("eg2_osc2_int", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg2_osc_intensity(sds, val);
                    }
                    else if (strncmp("eg3_attack", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_attack(sds, 3, val);
                    }
                    else if (strncmp("eg3_decay", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_decay(sds, 3, val);
                    }
                    else if (strncmp("eg3_sustain_ms", wurds[2], 14) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_sustain_ms(sds, 3, val);
                    }
                    else if (strncmp("eg3_sustain_level", wurds[2], 17) == 0)
                    {
                        printf("Sustain LEvel!\n");
                        double val = atof(wurds[3]);
                        eg_set_sustain_level(&sds->m_eg3, val);
                    }
                    else if (strncmp("eg3_release", wurds[2], 10) == 0)
                    {
                        double val = atof(wurds[3]);
                        synthdrum_set_eg_release(sds, 3, val);
                    }
                    else if (strncmp("filter_type", wurds[2], 11) == 0)
                    {
                        printf("filter tyyyype!\n");
                        int val = atof(wurds[3]);
                        synthdrum_set_filter_type(sds, val);
                    }
                    else if (strncmp("freq", wurds[2], 4) == 0)
                    {
                        printf("Freq!\n");
                        double val = atof(wurds[3]);
                        synthdrum_set_filter_freq(sds, val);
                    }
                    else if (strncmp("q", wurds[2], 1) == 0)
                    {
                        printf("Q!\n");
                        double val = atof(wurds[3]);
                        synthdrum_set_filter_q(sds, val);
                    }
                    else if (strncmp("distortion_threshold", wurds[2], 20) == 0)
                    {
                        printf("DIZZTORION!\n");
                        double val = atof(wurds[3]);
                        synthdrum_set_distortion_threshold(sds, val);
                    }
                    else if (strncmp("mod_pitch_semitones", wurds[2], 19) == 0)
                    {
                        printf("modPITCH!!\n");
                        int val = atoi(wurds[3]);
                        synthdrum_set_mod_semitones_range(sds, val);
                    }
                    else if (strncmp("save", wurds[2], 4) == 0 ||
                             strncmp("export", wurds[2], 6) == 0)
                    {
                        printf("Saving SYNTHDRUM patch..\n");
                        synthdrum_save_patch(sds, wurds[3]);
                    }
                    else if (strncmp("open", wurds[2], 4) == 0 ||
                             strncmp("import", wurds[2], 6) == 0 ||
                             strncmp("load", wurds[2], 4) == 0)
                    {
                        printf("Opening SYNTHDRUM "
                               "patches.. \n");
                        synthdrum_open_patch(sds, wurds[3]);
                    }
                    else
                    {
                        printf("SYNTHDRUM SEQ!\n");
                        parse_sequencer_command(&sds->m_seq, wurds, num_wurds,
                                                pattern);
                    }
                }
            }
        }

        //////  STEP SEQUENCER COMMANDS  /////////////////////////
        else if (strncmp("seq", wurds[0], 3) == 0)
        {

            char *pattern = (char *)calloc(151, sizeof(char));

            if (is_valid_file(wurds[1]))
            {
                sample_sequencer *s = new_sample_seq(wurds[1]);
                char_array_to_seq_string_pattern(&s->m_seq, pattern, wurds, 2,
                                                 num_wurds);
                int sgnum = add_sound_generator(
                    mixr,
                    (SOUNDGEN *)s); //  add_seq_char_pattern(mixr,
                                    //  wurds[1], pattern);
                pattern_char_to_pattern(
                    &s->m_seq, pattern,
                    s->m_seq.patterns[s->m_seq.num_patterns++]);
                mixr->midi_control_destination = MIDISEQUENCER;
                mixr->active_midi_soundgen_num = sgnum;
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        SEQUENCER_TYPE)
                {

                    if (strncmp("midi", wurds[2], 4) == 0)
                    {
                        printf("MIDI goes to Da Winner .. "
                               "Sequencer %d\n",
                               soundgen_num);
                        mixr->midi_control_destination = MIDISEQUENCER;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else if (strncmp("load", wurds[2], 4) == 0 ||
                             strncmp("open", wurds[2], 4) == 0 ||
                             strncmp("import", wurds[2], 6) == 0)
                    {
                        if (is_valid_file(wurds[3]))
                        {
                            printf("Changing Loaded "
                                   "FILE!\n");
                            sample_sequencer *s =
                                (sample_sequencer *)
                                    mixr->sound_generators[soundgen_num];
                            sample_seq_import_file(s, wurds[3]);
                        }
                    }
                    else
                    {
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

        else if (strncmp("spork", wurds[0], 5) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                mixr->sound_generators[soundgen_num]->type == SPORK_TYPE)
            {
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
        // GRANULATOR COMMANDS
        else if (strncmp("granulator", wurds[0], 8) == 0 ||
                 strncmp("gran", wurds[0], 4) == 0)
        {
            if (is_valid_file(wurds[1]) || strncmp(wurds[1], "none", 4) == 0)
            {
                printf("VALID!\n");
                int soundgen_num = add_granulator(mixr, wurds[1]);
                printf("SOUNDGEN %d\n", soundgen_num);
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        GRANULATOR_TYPE)
                {
                    granulator *g =
                        (granulator *)mixr->sound_generators[soundgen_num];
                    if (strncmp("grain_duration_ms", wurds[2], 14) == 0)
                    {
                        int dur = atoi(wurds[3]);
                        granulator_set_grain_duration(g, dur);
                    }
                    else if (strncmp("grains_per_sec", wurds[2], 14) == 0)
                    {
                        int gps = atoi(wurds[3]);
                        granulator_set_grains_per_sec(g, gps);
                    }
                    else if (strncmp("grain_file_pos", wurds[2], 14) == 0)
                    {
                        int pos = atoi(wurds[3]);
                        granulator_set_grain_buffer_position(g, pos);
                    }
                    else if (strncmp("grain_spray_ms", wurds[2], 14) == 0)
                    {
                        int spray = atoi(wurds[3]);
                        granulator_set_granular_spray(g, spray);
                    }
                    else if (strncmp("quasi_grain_fudge", wurds[2], 17) == 0)
                    {
                        int fudge = atoi(wurds[3]);
                        granulator_set_quasi_grain_fudge(g, fudge);
                    }
                    else if (strncmp("extsource", wurds[2], 9) == 0)
                    {
                        int sg = atoi(wurds[3]);
                        if (mixer_is_valid_soundgen_num(mixr, sg))
                        {
                            printf("GRAN is following "
                                   "%d\n",
                                   sg);
                            granulator_set_external_source(g, sg);
                        }
                    }
                    else if (strncmp("file", wurds[2], 4) == 0 ||
                             strncmp("open", wurds[2], 4) == 0 ||
                             strncmp("import", wurds[2], 6) == 0)
                    {
                        if (is_valid_file(wurds[3]))
                        {
                            granulator_import_file(g, wurds[3]);
                        }
                    }
                    else if (strncmp("grain_pitch", wurds[2], 10) == 0)
                    {
                        int pitch = atoi(wurds[3]);
                        granulator_set_grain_pitch(g, pitch);
                    }
                    else if (strncmp("selection_mode", wurds[2], 14) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        granulator_set_selection_mode(g, mode);
                    }
                    else if (strncmp("sequencer_mode", wurds[2], 13) == 0)
                    {
                        int mode = atoi(wurds[3]);
                        printf("MODE is %d\n", mode);
                        granulator_set_sequencer_mode(g, mode);
                    }
                    else if (strncmp("lfo1_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", mode);
                        granulator_set_lfo_voice(g, 1, type);
                    }
                    else if (strncmp("lfo1_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        granulator_set_lfo_amp(g, 1, amp);
                    }
                    else if (strncmp("lfo1_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        granulator_set_lfo_rate(g, 1, rate);
                    }
                    else if (strncmp("lfo1_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        granulator_set_lfo_sync(g, 1, loops);
                    }
                    else if (strncmp("lfo1_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        granulator_set_lfo_min(g, 1, min);
                    }
                    else if (strncmp("lfo1_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        granulator_set_lfo_max(g, 1, max);
                    }
                    else if (strncmp("lfo2_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", mode);
                        granulator_set_lfo_voice(g, 2, type);
                    }
                    else if (strncmp("lfo2_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        granulator_set_lfo_amp(g, 2, amp);
                    }
                    else if (strncmp("lfo2_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        granulator_set_lfo_rate(g, 2, rate);
                    }
                    else if (strncmp("lfo2_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        granulator_set_lfo_sync(g, 2, loops);
                    }
                    else if (strncmp("lfo2_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        granulator_set_lfo_min(g, 2, min);
                    }
                    else if (strncmp("lfo2_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        granulator_set_lfo_max(g, 2, max);
                    }
                    else if (strncmp("lfo3_type", wurds[2], 8) == 0)
                    {
                        int type = atoi(wurds[3]);
                        printf("LFO TYPE is %d\n", mode);
                        granulator_set_lfo_voice(g, 3, type);
                    }
                    else if (strncmp("lfo3_amp", wurds[2], 7) == 0)
                    {
                        double amp = atof(wurds[3]);
                        granulator_set_lfo_amp(g, 3, amp);
                    }
                    else if (strncmp("lfo3_rate", wurds[2], 8) == 0)
                    {
                        double rate = atof(wurds[3]);
                        granulator_set_lfo_rate(g, 3, rate);
                    }
                    else if (strncmp("lfo3_sync", wurds[2], 8) == 0)
                    {
                        double loops = atof(wurds[3]);
                        granulator_set_lfo_sync(g, 3, loops);
                    }
                    else if (strncmp("lfo3_min", wurds[2], 8) == 0)
                    {
                        double min = atof(wurds[3]);
                        granulator_set_lfo_min(g, 3, min);
                    }
                    else if (strncmp("lfo3_max", wurds[2], 8) == 0)
                    {
                        double max = atof(wurds[3]);
                        granulator_set_lfo_max(g, 3, max);
                    }
                    else if (strncmp("graindur_lfo_on", wurds[2], 14) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->graindur_lfo_on = b;
                    }
                    else if (strncmp("grainps_lfo_on", wurds[2], 13) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainps_lfo_on = b;
                    }
                    else if (strncmp("grainscan_lfo_on", wurds[2], 16) == 0)
                    {
                        int b = atoi(wurds[3]);
                        g->grainscanfile_lfo_on = b;
                    }
                    else if (strncmp("eg_amp_attack_ms", wurds[2], 16) == 0)
                    {
                        int attack = atoi(wurds[3]);
                        eg_set_attack_time_msec(&g->m_eg1, attack);
                    }
                    else if (strncmp("eg_amp_release_ms", wurds[2], 17) == 0)
                    {
                        int release = atoi(wurds[3]);
                        eg_set_release_time_msec(&g->m_eg1, release);
                    }
                    // else if (strncmp("eg_state", wurds[2], 8)
                    // == 0) {
                    //    //int state = atoi(wurds[3]);
                    //    // eg_set_state(&g->m_eg1, state);
                    //}
                    else
                    {
                        printf("ELSEY SEQUENCE!\n");
                        char *pattern = (char *)calloc(151, sizeof(char));
                        sequencer *seq = &g->m_seq;
                        parse_sequencer_command(seq, wurds, num_wurds, pattern);
                        free(pattern);
                    }
                }
            }
        }

        // SAMPLE LOOPER COMMANDS
        else if (strncmp("loop", wurds[0], 4) == 0)
        {
            if (is_valid_file(wurds[1]) || strncmp(wurds[1], "none", 4) == 0)
            {
                int loop_len = atoi(wurds[2]);
                // if (loop_len > 0) {
                int soundgen_num = add_looper(mixr, wurds[1], loop_len);
                printf("SOUNDGEN %d\n", soundgen_num);
                mixr->midi_control_destination = MIDILOOPER;
                mixr->active_midi_soundgen_num = soundgen_num;
                //}
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type == LOOPER_TYPE)
                {

                    looper *s = (looper *)mixr->sound_generators[soundgen_num];

                    if (strncmp("add", wurds[2], 6) == 0)
                    {
                        if (is_valid_file(wurds[3]) ||
                            strncmp(wurds[3], "none", 4) == 0)
                        {
                            int loop_len = atoi(wurds[4]);
                            if (loop_len > 0)
                            {
                                looper_add_sample(s, wurds[3], loop_len);
                            }
                        }
                    }
                    else if (strncmp("change", wurds[2], 6) == 0)
                    {
                        int sample_num = atoi(wurds[3]);
                        if (is_valid_sample_num(s, sample_num))
                        {
                            if (strncmp("looplen", wurds[4], 8) == 0)
                            {
                                int looplen = atoi(wurds[5]);
                                s->pending_loop_num = sample_num;
                                s->pending_loop_size = looplen;
                                s->change_loopsize_pending = true;
                                printf("CHANGEING "
                                       "PENDING "
                                       "..\n");
                            }
                            else if (strncmp("numloops", wurds[4], 8) == 0)
                            {
                                int numloops = atoi(wurds[5]);
                                if (numloops != 0)
                                {
                                    looper_change_num_loops(s, sample_num,
                                                            numloops);
                                }
                            }
                        }
                    }
                    else if (strncmp("midi", wurds[2], 4) == 0)
                    {
                        mixr->midi_control_destination = MIDILOOPER;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else if (strncmp("multi", wurds[2], 5) == 0)
                    {
                        if (strncmp("true", wurds[3], 4) == 0)
                        {
                            looper_set_multi_sample_mode(s, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0)
                        {
                            looper_set_multi_sample_mode(s, false);
                        }
                        printf("Sampler multi mode : %s\n",
                               s->multi_sample_mode ? "true" : "false");
                    }
                    else if (strncmp("scramble", wurds[2], 8) == 0)
                    {
                        if (strncmp(wurds[3], "every", 5) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                printf("Scrambling "
                                       "every %d n "
                                       "loops!\n",
                                       num_gens);
                                looper_set_scramble_mode(s, true);
                                s->scramble_every_n_loops = num_gens;
                            }
                            else
                            {
                                printf("Need a "
                                       "number for "
                                       "every "
                                       "'n'\n");
                            }
                        }
                        else
                        {
                            int max_gen = atoi(wurds[3]);
                            if (max_gen > 0)
                            {
                                looper_set_max_generation(s, max_gen);
                                looper_set_scramble_mode(s, true);
                            }
                            else
                            {
                                printf("Toggling "
                                       "scramble.."
                                       "\n");
                                int new_mode = 1 - s->scramblrrr_mode;
                                looper_set_scramble_mode(s, new_mode);
                            }
                        }
                    }
                    else if (strncmp("stutter", wurds[2], 7) == 0)
                    {
                        if (strncmp(wurds[3], "every", 4) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                printf("Stuttering "
                                       "every %d n "
                                       "loops!\n",
                                       num_gens);
                                looper_set_stutter_mode(s, true);
                                s->stutter_every_n_loops = num_gens;
                            }
                            else
                            {
                                printf("Need a "
                                       "number for "
                                       "every "
                                       "'n'\n");
                            }
                        }
                        else if (strncmp(wurds[3], "for", 3) == 0)
                        {
                            int max_gen = atoi(wurds[3]);
                            if (max_gen > 0)
                            {
                                looper_set_max_generation(s, max_gen);
                                looper_set_stutter_mode(s, true);
                            }
                            else
                            {
                                printf("Need a "
                                       "number of "
                                       "loops for "
                                       "'for'\n");
                            }
                        }
                        else if (strncmp(wurds[3], "true", 4) == 0)
                            looper_set_stutter_mode(s, true);
                        else if (strncmp(wurds[3], "false", 5) == 0)
                            looper_set_stutter_mode(s, false);
                        else
                        {
                            int new_mode = 1 - s->stutter_mode;
                            printf("Toggling sTUTTER "
                                   "to %s..\n",
                                   new_mode ? "true" : "false");
                            looper_set_stutter_mode(s, new_mode);
                        }
                    }
                    else if (strncmp("switch", wurds[3], 6) == 0)
                    {
                        int sample_num = atoi(wurds[3]);
                        looper_switch_sample(s, sample_num);
                    }
                }
            }
        }

        // SINGLE SHOT SAMPLE PLAYER COMMANDS
        else if (strncmp("play", wurds[0], 4) == 0)
        {
            printf("Playing onetime sample...\n");
        }

        // SYNTHESIZER COMMANDS
        else if (strncmp("syn", wurds[0], 3) == 0)
        {
            if (strncmp("mini", wurds[1], 4) == 0)
            {
                int sgnum = add_minisynth(mixr);
                mixr->midi_control_destination = SYNTH;
                mixr->active_midi_soundgen_num = sgnum;
                if (num_wurds > 2)
                {
                    minisynth *ms = (minisynth *)mixr->sound_generators[sgnum];
                    char_melody_to_midi_melody(&ms->base, 0, wurds, 2, num_wurds);
                }
            }
            else if (strncmp("digi", wurds[1], 4) == 0)
            {
                if (strlen(wurds[2]) != 0)
                {
                    int sgnum = add_digisynth(mixr, wurds[2]);
                    if (num_wurds > 2)
                    {
                        digisynth *ds = (digisynth *)mixr->sound_generators[sgnum];
                        char_melody_to_midi_melody(&ds->base, 0, wurds, 2, num_wurds);
                    }
                }
                else
                {
                    printf("Need to give me a sample name for a digisynth..\n");
                }
            }

            else if (strncmp("ls", wurds[1], 2) == 0)
            {
                minisynth_list_presets();
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        MINISYNTH_TYPE)
                {
                    minisynth *ms =
                        (minisynth *)mixr->sound_generators[soundgen_num];

                    if (strncmp("add", wurds[2], 3) == 0)
                    {
                        if (strncmp("melody", wurds[3], 6) == 0 ||
                            strncmp("pattern", wurds[3], 7) == 0)
                        {
                            int new_melody_num =
                                synthbase_add_melody(&ms->base);
                            if (num_wurds > 4)
                            {
                                char_melody_to_midi_melody(&ms->base, new_melody_num,
                                                           wurds, 4, num_wurds);
                            }
                        }
                    }
                    else if (strncmp("arp", wurds[2], 3) == 0)
                    {
                        ms->base.recording = false;
                        minisynth_set_arpeggiate(ms, 1 - ms->m_arp.active);
                    }
                    else if (strncmp("generate", wurds[2], 8) == 0)
                    {
                        int melody_num = atoi(wurds[3]);
                        int max_notes = atoi(wurds[4]);
                        int max_steps = atoi(wurds[5]);
                        synthbase_generate_melody(&ms->base, melody_num,
                                                  max_notes, max_steps);
                    }
                    else if (strncmp("rand", wurds[2], 4) == 0)
                    {
                        minisynth_rand_settings(ms);
                    }
                    else if (strncmp("genrand", wurds[2], 4) == 0)
                    {
                        minisynth_rand_settings(ms);
                        int melody_num = atoi(wurds[3]);
                        int max_notes = atoi(wurds[4]);
                        int max_steps = atoi(wurds[5]);
                        synthbase_generate_melody(&ms->base, melody_num,
                                                  max_notes, max_steps);
                    }
                    else if (strncmp("change", wurds[2], 6) == 0)
                    {
                        if (parse_minisynth_settings_change(ms, wurds))
                        {
                            continue;
                        }
                        int melody_num = atoi(wurds[3]);
                        if (is_valid_melody_num(&ms->base, melody_num))
                        {
                            if (strncmp("numloops", wurds[4], 8) == 0)
                            {
                                int numloops = atoi(wurds[5]);
                                if (numloops != 0)
                                {
                                    synthbase_set_melody_loop_num(
                                        &ms->base, melody_num, numloops);
                                    printf("NUMLOO"
                                           "PS "
                                           "Now "
                                           "%d\n",
                                           numloops);
                                }
                            }
                            else if (strncmp("add", wurds[4], 3) == 0)
                            {
                                int tick = 0;
                                int midi_note = 0;
                                sscanf(wurds[5], "%d:%d", &tick, &midi_note);
                                if (midi_note != 0)
                                {
                                    printf("Adding"
                                           " note"
                                           "\n");
                                    synthbase_add_note(&ms->base, melody_num,
                                                       tick, midi_note);
                                }
                            }
                            else if (strncmp("mv", wurds[4], 2) == 0)
                            {
                                int fromtick = atoi(wurds[5]);
                                int totick = atoi(wurds[6]);
                                printf("MV'ing "
                                       "note\n");
                                synthbase_mv_note(&ms->base, melody_num,
                                                  fromtick, totick);
                            }
                            else if (strncmp("rm", wurds[4], 2) == 0)
                            {
                                int tick = atoi(wurds[5]);
                                printf("Rm'ing "
                                       "note\n");
                                synthbase_rm_note(&ms->base, melody_num, tick);
                            }
                            else if (strncmp("madd", wurds[4], 4) == 0)
                            {
                                int tick = 0;
                                int midi_note = 0;
                                sscanf(wurds[5], "%d:%d", &tick, &midi_note);
                                if (midi_note != 0)
                                {
                                    printf("MAddin"
                                           "g "
                                           "note"
                                           "\n");
                                    synthbase_add_micro_note(
                                        &ms->base, melody_num, tick, midi_note);
                                }
                            }
                            else if (strncmp("melody", wurds[4], 6) == 0 ||
                                     strncmp("pattern", wurds[4], 7) == 0)
                            {
                                synthbase_reset_melody(&ms->base, melody_num);
                                char_melody_to_midi_melody(&ms->base, melody_num,
                                                           wurds, 5, num_wurds);
                            }
                            else if (strncmp("mmv", wurds[4], 2) == 0)
                            {
                                int fromtick = atoi(wurds[5]);
                                int totick = atoi(wurds[6]);
                                printf("MMV'ing "
                                       "note\n");
                                synthbase_mv_micro_note(&ms->base, melody_num,
                                                        fromtick, totick);
                            }
                            else if (strncmp("mrm", wurds[4], 3) == 0)
                            {
                                int tick = atoi(wurds[5]);
                                printf("Rm'ing "
                                       "note\n");
                                synthbase_rm_micro_note(&ms->base, melody_num,
                                                        tick);
                            }
                        }
                    }
                    else if (strncmp("cp", wurds[2], 2) == 0)
                    {
                        int pattern_num = atoi(wurds[3]);
                        int sg2 = 0;
                        int pattern_num2 = 0;
                        sscanf(wurds[4], "%d:%d", &sg2, &pattern_num2);
                        if (mixer_is_valid_soundgen_num(mixr, sg2) &&
                            mixr->sound_generators[sg2]->type == MINISYNTH_TYPE)
                        {
                            minisynth *ms2 =
                                (minisynth *)mixr->sound_generators[sg2];
                            if (is_valid_melody_num(&ms->base, pattern_num) &&
                                is_valid_melody_num(&ms2->base, pattern_num2))
                            {

                                printf("Copying SYNTH "
                                       "pattern from "
                                       "%d:%d to "
                                       "%d:%d!\n",
                                       soundgen_num, pattern_num, sg2,
                                       pattern_num2);

                                midi_event **melody = synthbase_copy_midi_loop(
                                    &ms->base, pattern_num);

                                synthbase_replace_midi_loop(&ms2->base, melody,
                                                            pattern_num2);
                            }
                        }
                    }
                    else if (strncmp("delete", wurds[2], 3) == 0)
                    {
                        if (strncmp("melody", wurds[3], 6) == 0)
                        {
                            // minisynth_delete_melody(ms);
                            // // TODO implement
                            printf("Imagine i deleted "
                                   "your melody..\n");
                        }
                        else
                        {
                            int melody = atoi(wurds[3]);
                            if (is_valid_melody_num(&ms->base, melody))
                            {
                                printf("MELODY "
                                       "DELETE "
                                       "EVENT!\n");
                            }
                            int tick = atoi(wurds[4]);
                            if (tick < PPNS)
                                synthbase_rm_micro_note(&ms->base, melody,
                                                        tick);
                        }
                    }
                    else if (strncmp("dupe", wurds[2], 4) == 0)
                    {
                        int pattern_num = atoi(wurds[3]);
                        int new_pattern_num = synthbase_add_melody(&ms->base);
                        synthbase_dupe_melody(
                            ms->base.melodies[pattern_num],
                            ms->base.melodies[new_pattern_num]);
                    }
                    else if (strncmp("import", wurds[2], 6) == 0)
                    {
                        printf("Importing file\n");
                        synthbase_import_midi_from_file(&ms->base, wurds[3]);
                    }
                    else if (strncmp("keys", wurds[2], 4) == 0)
                    {
                        keys(soundgen_num);
                    }
                    else if (strncmp("load", wurds[2], 4) == 0)
                    {
                        char preset_name[20];
                        strncpy(preset_name, wurds[3], 19);
                        minisynth_load_settings(ms, preset_name);
                    }
                    else if (strncmp("midi", wurds[2], 4) == 0)
                    {
                        mixr->midi_control_destination = SYNTH;
                        mixr->active_midi_soundgen_num = soundgen_num;
                    }
                    else if (strncmp("genmode", wurds[2], 8) == 0)
                    {
                        if (strncmp("every", wurds[3], 5) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                synthbase_set_generate_mode(&ms->base, true);
                                ms->base.morph_every_n_loops = num_gens;
                            }
                            else
                            {
                                printf("Need a "
                                       "number for "
                                       "every "
                                       "'n'\n");
                            }
                        }
                        else if (strncmp("for", wurds[3], 3) == 0)
                        {
                            int num_gens = atoi(wurds[4]);
                            if (num_gens > 0)
                            {
                                synthbase_set_generate_mode(&ms->base, true);
                                ms->base.max_generation = num_gens;
                            }
                            else
                            {
                                printf("Need a "
                                       "number for "
                                       "'for'\n");
                            }
                        }
                        else
                        { // just toggle
                            synthbase_set_generate_mode(
                                &ms->base, 1 - ms->base.generate_mode);
                        }

                        printf("Synth GENERATE mode : %s\n",
                               ms->base.generate_mode ? "true" : "false");
                    }
                    else if (strncmp("multi", wurds[2], 5) == 0)
                    {
                        if (strncmp("true", wurds[3], 4) == 0)
                        {
                            synthbase_set_multi_melody_mode(&ms->base, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0)
                        {
                            synthbase_set_multi_melody_mode(&ms->base, false);
                        }
                        printf("Synth multi mode : %s\n",
                               ms->base.multi_melody_mode ? "true" : "false");
                    }
                    else if (strncmp("nudge", wurds[2], 5) == 0)
                    {
                        int sixteenth = atoi(wurds[3]);
                        if (sixteenth < 16)
                        {
                            printf("Nudging Melody "
                                   "along %d "
                                   "sixteenthzzzz!\n",
                                   sixteenth);
                            synthbase_nudge_melody(
                                &ms->base, ms->base.cur_melody, sixteenth);
                        }
                    }
                    else if (strncmp("print", wurds[2], 5) == 0)
                    {
                        minisynth_print(ms);
                    }
                    else if (strncmp("quantize", wurds[2], 8) == 0)
                    {
                        int melody_num = atoi(wurds[3]);
                        if (is_valid_melody_num(&ms->base, melody_num))
                        {
                            printf("QuantiZe!\n");
                            midi_event **melody =
                                ms->base.melodies[ms->base.cur_melody];
                            midi_melody_quantize(melody);
                        }
                    }
                    else if (strncmp("rand", wurds[2], 4) == 0)
                    {
                        minisynth_rand_settings(ms);
                    }
                    else if (strncmp("reset", wurds[2], 5) == 0)
                    {
                        if (strncmp("all", wurds[3], 3) == 0)
                        {
                            synthbase_reset_melody_all(&ms->base);
                        }
                        else
                        {
                            int melody_num = atoi(wurds[3]);
                            synthbase_reset_melody(&ms->base, melody_num);
                        }
                    }
                    else if (strncmp("save", wurds[2], 4) == 0)
                    {
                        char preset_name[20];
                        strncpy(preset_name, wurds[3], 19);
                        minisynth_save_settings(ms, preset_name);
                    }
                    else if (strncmp("switch", wurds[2], 6) == 0 ||
                             strncmp("CurMelody", wurds[2], 9) == 0)
                    {
                        int melody_num = atoi(wurds[3]);
                        synthbase_switch_melody(&ms->base, melody_num);
                    }
                    else if (strncmp("sustain", wurds[2], 5) == 0)
                    {
                        if (strncmp("true", wurds[3], 4) == 0)
                        {
                            minisynth_set_sustain_override(ms, true);
                        }
                        else if (strncmp("false", wurds[3], 5) == 0)
                        {
                            minisynth_set_sustain_override(ms, false);
                        }
                        printf("Synth Sustain Override : %s\n",
                               ms->m_settings.m_sustain_override ? "true"
                                                                 : "false");
                        for (int i = 0; i < MAX_VOICES; i++)
                        {
                            if (ms->m_voices[i])
                            {
                                printf("EG sustain: "
                                       "%d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg1.m_sustain_override);
                                printf("EG sustain: "
                                       "%d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg2.m_sustain_override);
                                printf("EG sustain: "
                                       "%d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg3.m_sustain_override);
                                printf("EG sustain: "
                                       "%d\n",
                                       ms->m_voices[i]
                                           ->m_voice.m_eg4.m_sustain_override);
                            }
                        }
                    }
                }
            }
        }

        // CHAOS COMMANDS
        else if (strncmp("chaos", wurds[0], 6) == 0)
        {

            if (strncmp("monkey", wurds[1], 6) == 0)
            {
                int soundgen_num = atoi(wurds[2]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
                {
                    add_chaosmonkey(soundgen_num);
                }
            }
            else
            {
                int soundgen_num = atoi(wurds[1]);
                if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                    mixr->sound_generators[soundgen_num]->type ==
                        CHAOSMONKEY_TYPE)
                {

                    chaosmonkey *cm =
                        (chaosmonkey *)mixr->sound_generators[soundgen_num];

                    if (strncmp("wakeup", wurds[2], 7) == 0)
                    {
                        int freq_wakeup_secs = atoi(wurds[3]);
                        chaosmonkey_change_wakeup_freq(cm, freq_wakeup_secs);
                    }
                    else if (strncmp("chance", wurds[2], 7) == 0)
                    {
                        int percent_chance = atoi(wurds[3]);
                        chaosmonkey_change_chance_interrupt(cm, percent_chance);
                    }
                    else if (strncmp("suggest", wurds[2], 8) == 0)
                    {
                        if (strncmp("true", wurds[3], 5) == 0 ||
                            strncmp("false", wurds[3], 6) == 0)
                        {
                            bool val =
                                strcmp("true", wurds[4]) == 0 ? true : false;
                            chaosmonkey_suggest_mode(cm, val);
                        }
                    }
                    else if (strncmp("action", wurds[2], 7) == 0)
                    {
                        if (strncmp("true", wurds[3], 5) == 0 ||
                            strncmp("false", wurds[3], 6) == 0)
                        {
                            bool val =
                                strcmp("true", wurds[4]) == 0 ? true : false;
                            chaosmonkey_action_mode(cm, val);
                        }
                    }
                }
            }
        }
        // FX COMMANDS
        else if (strncmp("compressor", wurds[0], 10) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_compressor_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("delay", wurds[0], 7) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int delay_len_ms = atoi(wurds[2]);
                add_delay_soundgen(mixr->sound_generators[soundgen_num],
                                   delay_len_ms);
            }
        }
        else if (strncmp("filter", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_basicfilter_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("follower", wurds[0], 8) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_follower_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        // else if (strncmp("granulator", wurds[0], 8) == 0 ||
        //         strncmp("gran", wurds[0], 4) == 0) {
        //    int soundgen_num = atoi(wurds[1]);
        //    if (mixer_is_valid_soundgen_num(mixr, soundgen_num)) {
        //        add_granulator_soundgen(mixr->sound_generators[soundgen_num]);
        //    }
        //}
        else if (strncmp("moddelay", wurds[0], 7) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_moddelay_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("modfilter", wurds[0], 9) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_modfilter_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("reverb", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_reverb_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("waveshape", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_waveshape_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("distort", wurds[0], 7) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_distortion_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("decimate", wurds[0], 8) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                add_decimator_soundgen(mixr->sound_generators[soundgen_num]);
            }
        }
        else if (strncmp("env", wurds[0], 3) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int loop_len = atoi(wurds[2]);
                int env_type = atoi(wurds[3]);
                ENVSTREAM *e = new_envelope_stream(loop_len, env_type);
                if (e != NULL)
                {
                    add_envelope_soundgen(mixr->sound_generators[soundgen_num],
                                          e);
                }
            }
        }
        else if (strncmp("lowpass", wurds[0], 8) == 0 ||
                 strncmp("highpass", wurds[0], 9) == 0 ||
                 strncmp("bandpass", wurds[0], 9) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int val = atoi(wurds[2]);
                if (strcmp("lowpass", wurds[0]) == 0)
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, LOWPASS);
                else if (strcmp("highpass", wurds[0]) == 0)
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, HIGHPASS);
                else
                    add_freq_pass_soundgen(mixr->sound_generators[soundgen_num],
                                           val, BANDPASS);
            }
        }

        else if (strncmp("repeat", wurds[0], 6) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
            {
                int nbeats = atoi(wurds[2]);
                int sixteenth = atoi(wurds[3]);
                add_beatrepeat_soundgen(mixr->sound_generators[soundgen_num],
                                        nbeats, sixteenth);
            }
        }

        else if (strncmp("fx", wurds[0], 2) == 0)
        {
            int soundgen_num = atoi(wurds[1]);
            int fx_num = atoi(wurds[2]);
            if (is_valid_fx_num(soundgen_num, fx_num))
            {
                fx *f = mixr->sound_generators[soundgen_num]->effects[fx_num];

                if (strncmp("on", wurds[3], 2) == 0)
                    f->enabled = true;
                else if (strncmp("off", wurds[3], 3) == 0)
                    f->enabled = false;
                else if (f->type == DELAY)
                {
                    // printf("Changing Dulay!\n");
                    stereodelay *sd = (stereodelay *)f;
                    double val = atof(wurds[4]);
                    // keep these strings in sync with status()
                    // output
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
                else if (f->type == COMPRESSOR)
                {
                    dynamics_processor *dp = (dynamics_processor *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("inputgain", wurds[3], 9) == 0)
                        dynamics_processor_set_inputgain_db(dp, val);
                    else if (strncmp("threshold", wurds[3], 9) == 0)
                        dynamics_processor_set_threshold(dp, val);
                    else if (strncmp("attackms", wurds[3], 8) == 0)
                        dynamics_processor_set_attack_ms(dp, val);
                    else if (strncmp("releasems", wurds[3], 9) == 0)
                        dynamics_processor_set_release_ms(dp, val);
                    else if (strncmp("ratio", wurds[3], 5) == 0)
                        dynamics_processor_set_ratio(dp, val);
                    else if (strncmp("outputgain", wurds[3], 10) == 0)
                        dynamics_processor_set_outputgain_db(dp, val);
                    else if (strncmp("kneewidth", wurds[3], 9) == 0)
                        dynamics_processor_set_knee_width(dp, val);
                    else if (strncmp("lookahead", wurds[3], 9) == 0)
                        dynamics_processor_set_lookahead_delay_ms(dp, val);
                    else if (strncmp("stereolink", wurds[3], 9) == 0)
                        dynamics_processor_set_stereo_link(dp, val);
                    else if (strncmp("type", wurds[3], 4) == 0)
                        dynamics_processor_set_processor_type(dp, val);
                    else if (strncmp("mode", wurds[3], 4) == 0)
                        dynamics_processor_set_time_constant(dp, val);
                    else if (strncmp("extsource", wurds[3], 9) == 0)
                        dynamics_processor_set_external_source(dp, val);
                    else if (strncmp("def", wurds[3], 3) == 0 ||
                             strncmp("default", wurds[3], 7) == 0)
                        dynamics_processor_set_default_sidechain_params(dp);
                }
                // else if (f->type == GRANULATOR) {
                //    granulator *g = (granulator *)f;
                //    double val = atof(wurds[4]);
                //    if (strncmp("numgrains", wurds[3], 9) == 0)
                //        granulator_set_num_grains(g, val);
                //    else if (strncmp("grainlen", wurds[3], 8) ==
                //    0)
                //        granulator_set_grain_len(g, val);
                //    else if (strncmp("fudge", wurds[3], 5) == 0)
                //        granulator_set_fudge_factor(g, val);
                //    else if (strncmp("wetmx", wurds[3], 5) == 0)
                //        granulator_set_wet_mix(g, val);
                //    else if (strncmp("locked", wurds[3], 5) == 0)
                //        granulator_set_locked(g, val);
                //    else if (strncmp("read_pos", wurds[3], 8) ==
                //    0)
                //        granulator_set_read_pos(g, val);
                //    else if (strncmp("env", wurds[3], 3) == 0)
                //        granulator_set_apply_env(g, val);
                //}
                else if (f->type == REVERB)
                {
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
                else if (f->type == WAVESHAPER)
                {
                    waveshaper *ws = (waveshaper *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("k_pos", wurds[3], 5) == 0)
                        waveshaper_set_arc_tan_k_pos(ws, val);
                    else if (strncmp("k_neg", wurds[3], 5) == 0)
                        waveshaper_set_arc_tan_k_neg(ws, val);
                    else if (strncmp("stages", wurds[3], 5) == 0)
                        waveshaper_set_stages(ws, val);
                    else if (strncmp("invert", wurds[3], 5) == 0)
                        waveshaper_set_invert_stages(ws, val);
                }
                else if (f->type == BASICFILTER)
                {
                    filterpass *fp = (filterpass *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("freq", wurds[3], 4) == 0)
                        filter_set_fc_control(&fp->m_filter.f, val);
                    if (strncmp("q", wurds[3], 4) == 0)
                        moog_set_qcontrol(&fp->m_filter.f, val);
                    else if (strncmp("type", wurds[3], 4) == 0)
                        filter_set_type(&fp->m_filter.f, val);
                }
                else if (f->type == BEATREPEAT)
                {
                    beatrepeat *br = (beatrepeat *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("numbeats", wurds[3], 8) == 0)
                        beatrepeat_change_num_beats_to_repeat(br, val);
                    else if (strncmp("sixteenth", wurds[3], 9) == 0)
                        beatrepeat_change_selected_sixteenth(br, val);
                }
                else if (f->type == DISTORTION)
                {
                    distortion *d = (distortion *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("threshold", wurds[3], 9) == 0)
                        distortion_set_threshold(d, val);
                }
                else if (f->type == ENVELOPEFOLLOWER)
                {
                    envelope_follower *ef = (envelope_follower *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("pregain", wurds[3], 4) == 0)
                        envelope_follower_set_pregain_db(ef, val);
                    else if (strncmp("threshold", wurds[3], 9) == 0)
                        envelope_follower_set_threshold(ef, val);
                    else if (strncmp("attackms", wurds[3], 8) == 0)
                        envelope_follower_set_attack_ms(ef, val);
                    else if (strncmp("releasems", wurds[3], 9) == 0)
                        envelope_follower_set_release_ms(ef, val);
                    else if (strncmp("q", wurds[3], 1) == 0)
                        envelope_follower_set_q(ef, val);
                    else if (strncmp("mode", wurds[3], 4) == 0)
                        envelope_follower_set_time_constant(ef, val);
                    else if (strncmp("dir", wurds[3], 4) == 0)
                        envelope_follower_set_direction(ef, val);
                }

                else if (f->type == MODDELAY)
                {
                    mod_delay *md = (mod_delay *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("depth", wurds[3], 5) == 0)
                    {
                        mod_delay_set_depth(md, val);
                    }
                    else if (strncmp("rate", wurds[3], 4) == 0)
                    {
                        mod_delay_set_rate(md, val);
                    }
                    else if (strncmp("fb", wurds[3], 8) == 0)
                    {
                        mod_delay_set_feedback_percent(md, val);
                    }
                    else if (strncmp("offset", wurds[3], 12) == 0)
                    {
                        mod_delay_set_chorus_offset(md, val);
                    }
                    else if (strncmp("type", wurds[3], 7) == 0)
                    {
                        mod_delay_set_mod_type(md, (unsigned int)val);
                    }
                    else if (strncmp("lfo", wurds[3], 7) == 0)
                    {
                        mod_delay_set_lfo_type(md, (unsigned int)val);
                    }
                }
                else if (f->type == MODFILTER)
                {
                    modfilter *mf = (modfilter *)f;
                    double val = atof(wurds[4]);
                    if (strncmp("depthfc", wurds[3], 7) == 0)
                    {
                        modfilter_set_mod_depth_fc(mf, val);
                    }
                    else if (strncmp("ratefc", wurds[3], 6) == 0)
                    {
                        modfilter_set_mod_rate_fc(mf, val);
                    }
                    else if (strncmp("depthq", wurds[3], 6) == 0)
                    {
                        modfilter_set_mod_depth_q(mf, val);
                    }
                    else if (strncmp("rateq", wurds[3], 5) == 0)
                    {
                        modfilter_set_mod_rate_q(mf, val);
                    }
                    else if (strncmp("phase", wurds[3], 8) == 0)
                    {
                        modfilter_set_lfo_phase(mf, val);
                    }
                    else if (strncmp("lfo", wurds[3], 3) == 0)
                    {
                        modfilter_set_lfo_waveform(mf, val);
                    }
                }
            }
        }

        // PROGRAMMING CMDS
        else if (strncmp("var", wurds[0], 3) == 0 &&
                 strncmp("=", wurds[2], 1) == 0)
        {
            printf("Oooh! %s = %s\n", wurds[1], wurds[3]);
            update_environment(wurds[1], atoi(wurds[3]));
        }

        else if (strncmp("every", wurds[0], 5) == 0 &&
                 strncmp("loop", wurds[1], 4) == 0)
        {
            printf("Starting an algorithm - with %s!\n", cmd);
            add_algorithm(cmd);
        }

        // UTILS
        else if (strncmp("chord", wurds[0], 6) == 0)
        {
            chordie(wurds[1]);
        }

        else if (strncmp("strategy", wurds[0], 8) == 0)
        {
            oblique_strategy();
        }

        // default HALP!
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
        if (seq->pattern_len == 16)
        {
            seq_set_gridsteps(seq, 24);
        }
        strncat(dest_pattern,
                "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23",
                151);
    }
    else if (strncmp("all", char_array[start], 3) == 0)
    {
        if (seq->pattern_len == 16)
        {
            strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", 127);
        }
        else if (seq->pattern_len == 24)
        {
            strncat(dest_pattern, "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 "
                                  "15 16 17 18 19 20 21 22 23",
                    151);
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

bool is_valid_sample_num(looper *s, int sample_num)
{
    if (sample_num < s->num_samples)
    {
        return true;
    }
    return false;
}

bool is_valid_fx_num(int soundgen_num, int fx_num)
{
    if (mixer_is_valid_soundgen_num(mixr, soundgen_num))
    {
        if (mixr->sound_generators[soundgen_num]->effects_num > 0 &&
            fx_num < mixr->sound_generators[soundgen_num]->effects_num)
        {
            return true;
        }
    }
    return false;
}

bool is_valid_file(char *filename)
{
    printf("IS valid? %s\n", filename);
    if (strlen(filename) == 0)
    {
        printf("That ain't no valid file where i come from..\n");
        return false;
    }
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
    if (strncmp("add", wurds[2], 3) == 0)
    {
        printf("Adding\n");
        char_array_to_seq_string_pattern(seq, pattern, wurds, 3, num_wurds);
        add_char_pattern(seq, pattern);
    }
    else if (strncmp("grid", wurds[2], 4) == 0)
    {
        int gridsteps = atoi(wurds[3]);
        if (gridsteps != 16 && gridsteps != 24)
        {
            printf("Gridsteps must be either 16 or 24 (not %d)\n", gridsteps);
            return;
        }
        printf("Change gridsteps to %d\n", gridsteps);
        seq_set_gridsteps(seq, gridsteps);
    }
    else if (strncmp("sloppy", wurds[2], 6) == 0)
    {
        int sloppyjoe = atoi(wurds[3]);
        seq_set_sloppiness(seq, sloppyjoe);
    }
    else if (strncmp("print", wurds[2], 5) == 0)
    {
        int pattern_num = atoi(wurds[3]);
        if (seq_is_valid_pattern_num(seq, pattern_num))
        {
            printf("Printing pattern for %d\n", pattern_num);
            seq_print_pattern(seq, pattern_num);
        }
    }
    else if (strncmp("randamp", wurds[2], 6) == 0)
    {
        seq_set_randamp(seq, 1 - seq->randamp_on);
        printf("Toggling randamp to %s \n", seq->randamp_on ? "true" : "false");
    }
    else if (strncmp("multi", wurds[2], 5) == 0)
    {
        if (strncmp("true", wurds[3], 4) == 0)
        {
            seq_set_multi_pattern_mode(seq, true);
        }
        else if (strncmp("false", wurds[3], 5) == 0)
        {
            seq_set_multi_pattern_mode(seq, false);
        }
        printf("Sequencer multi mode : %s\n",
               seq->multi_pattern_mode ? "true" : "false");
    }
    // else if (strncmp("change", wurds[2], 6) == 0) {
    else if (strncmp("markov_mode", wurds[2], 11) == 0)
    {
        printf("MARKOV!\n");
        if (strncmp("haus", wurds[3], 4) == 0)
        {
            printf("HAUS!\n");
            seq_set_markov_mode(seq, MARKOVHAUS);
        }
        else if (strncmp("boombap", wurds[3], 7) == 0 ||
                 strncmp("hiphop", wurds[3], 6) == 0 ||
                 strncmp("beats", wurds[3], 5) == 0)
        {
            printf("BOOMBAP!\n");
            seq_set_markov_mode(seq, MARKOVBOOMBAP);
        }
        else if (strncmp("snare", wurds[3], 7) == 0)
        {
            printf("MARKOVSNARE!\n");
            seq_set_markov_mode(seq, MARKOVSNARE);
        }
    }
    else if (strncmp("life", wurds[2], 4) == 0)
    {
        if (strncmp("every", wurds[3], 5) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_game_of_life(seq, true);
                seq->life_every_n_loops = num_gens;
            }
            else
            {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_game_of_life(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else
            {
                printf("Need a number for 'for'\n");
            }
        }
        else
        {
            seq_set_game_of_life(seq, 1 - seq->game_of_life_on);
        }
    }
    else if (strncmp("markov", wurds[2], 4) == 0)
    {
        if (strncmp("every", wurds[3], 5) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_markov(seq, true);
                seq->markov_every_n_loops = num_gens;
            }
            else
            {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_markov(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else
            {
                printf("Need a number for 'for'\n");
            }
        }
        else
        {
            seq_set_markov(seq, 1 - seq->markov_on);
        }
    }
    else if (strncmp("shuffle", wurds[2], 7) == 0)
    {
        if (strncmp("every", wurds[3], 5) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_shuffle(seq, true);
                seq->shuffle_every_n_loops = num_gens;
            }
            else
            {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_shuffle(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else
            {
                printf("Need a number for 'for'\n");
            }
        }
        else
        {
            seq_set_shuffle(seq, 1 - seq->shuffle_on);
        }
    }
    else if (strncmp("bitwise", wurds[2], 4) == 0)
    {
        if (strncmp("every", wurds[3], 5) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_bitwise(seq, true);
                seq->bitwise_every_n_loops = num_gens;
            }
            else
            {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_bitwise(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else
            {
                printf("Need a number for 'for'\n");
            }
        }
        else
        {
            seq_set_bitwise(seq, 1 - seq->game_of_life_on);
        }
    }
    else if (strncmp("generate", wurds[2], 8) == 0)
    {
        int pattern_num = atoi(wurds[3]);
        next_euclidean_generation(seq, pattern_num);
    }
    else if (strncmp("euclid", wurds[2], 6) == 0)
    {
        printf("Euclidean, yo!\n");
        if (strncmp("every", wurds[3], 5) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_euclidean(seq, true);
                seq->euclidean_every_n_loops = num_gens;
            }
            else
            {
                printf("Need a number for every 'n'\n");
            }
        }
        else if (strncmp("for", wurds[3], 3) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_euclidean(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else
            {
                printf("Need a number for 'for'\n");
            }
        }
        else
        {
            seq_set_euclidean(seq, 1 - seq->game_of_life_on);
        }
    }
    else
    {
        int pattern_num = atoi(wurds[3]);
        if (seq_is_valid_pattern_num(seq, pattern_num))
        {
            if (strncmp("amp", wurds[4], 3) == 0)
            {
                int hit = atoi(wurds[5]);
                double amp = atof(wurds[6]);
                printf("Changing amp of %d:%d to %f\n", pattern_num, hit, amp);
                seq_set_sample_amp(seq, pattern_num, hit, amp);
            }
            if (strncmp("add", wurds[4], 3) == 0)
            {
                int hit = atoi(wurds[5]);
                printf("Adding a hit to %d\n", hit);
                seq_add_hit(seq, pattern_num, hit);
            }
            else if (strncmp("madd", wurds[4], 3) == 0)
            { // midi pulses
                int hit = atoi(wurds[5]);
                printf("Adding a hit to %d\n", hit);
                seq_add_micro_hit(seq, pattern_num, hit);
            }
            else if (strncmp("amp", wurds[4], 3) == 0)
            {
                char_array_to_seq_string_pattern(seq, pattern, wurds, 5,
                                                 num_wurds);
                printf("Setting pattern AMP to %s\n", pattern);
                seq_set_sample_amp_from_char_pattern(seq, pattern_num, pattern);
            }
            else if (strncmp("mv", wurds[4], 2) == 0)
            { // deals in 16th or 24th
                int hitfrom = atoi(wurds[5]);
                int hitto = atoi(wurds[6]);
                seq_mv_hit(seq, pattern_num, hitfrom, hitto);
            }
            else if (strncmp("mmv", wurds[4], 2) == 0)
            { // deals in midi pulses
                int hitfrom = atoi(wurds[5]);
                int hitto = atoi(wurds[6]);
                seq_mv_micro_hit(seq, pattern_num, hitfrom, hitto);
            }
            else if (strncmp("numloops", wurds[4], 8) == 0)
            {
                int numloops = atoi(wurds[5]);
                if (numloops != 0)
                {
                    seq_change_num_loops(seq, pattern_num, numloops);
                }
            }
            else if (strncmp("pattern", wurds[4], 7) == 0)
            {
                char_array_to_seq_string_pattern(seq, pattern, wurds, 5,
                                                 num_wurds);
                printf("Changing pattern to %s\n", pattern);
                change_char_pattern(seq, pattern_num, pattern);
            }
            else if (strncmp("rm", wurds[4], 2) == 0)
            {
                int hit = atoi(wurds[5]);
                printf("Rm'ing hit to %d\n", hit);
                seq_rm_hit(seq, pattern_num, hit);
            }
            else if (strncmp("mrm", wurds[4], 2) == 0)
            {
                int hit = atoi(wurds[5]);
                printf("Rm'ing hit to %d\n", hit);
                seq_rm_micro_hit(seq, pattern_num, hit);
            }
            else if (strncmp("swing", wurds[4], 5) == 0)
            {
                int swing_setting = atoi(wurds[5]);
                printf("changing swing to %d for pattern num %d\n",
                       swing_setting, pattern_num);
                seq_swing_pattern(seq, pattern_num, swing_setting);
            }
        }
    }
}

bool parse_minisynth_settings_change(minisynth *ms, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("attackms", wurds[3], 8) == 0)
    {
        printf("Minisynth change Attack Time Ms!\n");
        double val = atof(wurds[4]);
        minisynth_set_attack_time_ms(ms, val);
        return true;
    }
    else if (strncmp("decayms", wurds[3], 7) == 0)
    {
        printf("Minisynth change Decay Time MS!\n");
        double val = atof(wurds[4]);
        minisynth_set_decay_time_ms(ms, val);
        return true;
    }
    else if (strncmp("releasems", wurds[3], 7) == 0)
    {
        printf("Minisynth change Release Time MS!\n");
        double val = atof(wurds[4]);
        minisynth_set_release_time_ms(ms, val);
        return true;
    }
    else if (strncmp("delayfb", wurds[3], 7) == 0)
    {
        printf("Minisynth change Delay Feedback!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_feedback_pct(ms, val);
        return true;
    }
    else if (strncmp("delayr", wurds[3], 6) == 0)
    {
        printf("Minisynth change Delay Ratio!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_ratio(ms, val);
        return true;
    }
    else if (strncmp("delaymode", wurds[3], 9) == 0)
    {
        printf("Minisynth change DELAY MODE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_delay_mode(ms, val);
        return true;
    }
    else if (strncmp("delayms", wurds[3], 7) == 0)
    {
        printf("Minisynth change Delay Time Ms!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_time_ms(ms, val);
        return true;
    }
    else if (strncmp("delaymx", wurds[3], 7) == 0)
    {
        printf("Minisynth change Delay Wet Mix!\n");
        double val = atof(wurds[4]);
        minisynth_set_delay_wetmix(ms, val);
        return true;
    }
    else if (strncmp("detune", wurds[3], 6) == 0)
    {
        printf("Minisynth change DETUNE!\n");
        double val = atof(wurds[4]);
        minisynth_set_detune(ms, val);
        return true;
    }
    else if (strncmp("eg1dcaint", wurds[3], 9) == 0)
    {
        printf("Minisynth change EG1 DCA Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_eg1_dca_int(ms, val);
        return true;
    }
    else if (strncmp("eg1filterint", wurds[3], 12) == 0)
    {
        printf("Minisynth change EG1 Filter Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_eg1_filter_int(ms, val);
        return true;
    }
    else if (strncmp("eg1oscint", wurds[3], 9) == 0)
    {
        printf("Minisynth change EG1 Osc Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_eg1_osc_int(ms, val);
        return true;
    }
    else if (strncmp("fc", wurds[3], 2) == 0)
    {
        printf("Minisynth change Filter Cutoff!\n");
        double val = atof(wurds[4]);
        minisynth_set_filter_fc(ms, val);
        return true;
    }
    else if (strncmp("fq", wurds[3], 2) == 0)
    {
        printf("Minisynth change Filter Qualivity!\n");
        double val = atof(wurds[4]);
        minisynth_set_filter_fq(ms, val);
        return true;
    }
    else if (strncmp("filtertype", wurds[3], 4) == 0)
    {
        printf("Minisynth change Filter TYPE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_filter_type(ms, val);
        return true;
    }
    else if (strncmp("saturation", wurds[3], 10) == 0)
    {
        printf("Minisynth change Filter SATURATION!\n");
        double val = atof(wurds[4]);
        minisynth_set_filter_saturation(ms, val);
        return true;
    }
    else if (strncmp("nlp", wurds[3], 3) == 0)
    {
        printf("Minisynth change Filter NLP!\n");
        unsigned val = atoi(wurds[4]);
        minisynth_set_filter_nlp(ms, val);
        return true;
    }
    else if (strncmp("ktint", wurds[3], 5) == 0)
    {
        printf("Minisynth change Filter Keytrack Intensity!\n");
        double val = atof(wurds[4]);
        minisynth_set_keytrack_int(ms, val);
        return true;
    }
    else if (strncmp("kt", wurds[3], 2) == 0)
    {
        printf("Minisynth change Filter Keytrack!\n");
        int val = atoi(wurds[4]);
        minisynth_set_keytrack(ms, val);
        return true;
    }
    else if (strncmp("legato", wurds[3], 6) == 0)
    {
        printf("Minisynth change LEGATO!\n");
        int val = atoi(wurds[4]);
        minisynth_set_legato_mode(ms, val);
        return true;
    }
    else if (strncmp("lfo1wave", wurds[3], 7) == 0)
    {
        printf("Minisynth change LFO1 Wave!\n");
        int val = atoi(wurds[4]);
        minisynth_set_lfo_wave(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1ampint", wurds[3], 10) == 0)
    {
        printf("Minisynth change LFO1 Amp Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_amp_int(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1amp", wurds[3], 7) == 0)
    {
        printf("Minisynth change LFO1 AMP!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_amp(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1filterint", wurds[3], 13) == 0)
    {
        printf("Minisynth change LFO1 Filter FC Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_filter_fc_int(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1rate", wurds[3], 8) == 0)
    {
        printf("Minisynth change LFO1 rate!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_rate(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1panint", wurds[3], 10) == 0)
    {
        printf("Minisynth change LFO1 Pan Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_pan_int(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo1pitch", wurds[3], 9) == 0)
    {
        printf("Minisynth change LFO1 Pitch!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_pitch(ms, 1, val);
        return true;
    }
    else if (strncmp("lfo2wave", wurds[3], 7) == 0)
    {
        printf("Minisynth change LFO2 Wave!\n");
        int val = atoi(wurds[4]);
        minisynth_set_lfo_wave(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2ampint", wurds[3], 10) == 0)
    {
        printf("Minisynth change LFO2 Amp Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_amp_int(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2amp", wurds[3], 7) == 0)
    {
        printf("Minisynth change LFO2 AMP!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_amp(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2filterint", wurds[3], 13) == 0)
    {
        printf("Minisynth change LFO2 Filter FC Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_filter_fc_int(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2rate", wurds[3], 8) == 0)
    {
        printf("Minisynth change LFO2 rate!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_rate(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2panint", wurds[3], 10) == 0)
    {
        printf("Minisynth change LFO2 Pan Int!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_pan_int(ms, 2, val);
        return true;
    }
    else if (strncmp("lfo2pitch", wurds[3], 9) == 0)
    {
        printf("Minisynth change LFO2 Pitch!\n");
        double val = atof(wurds[4]);
        minisynth_set_lfo_pitch(ms, 2, val);
        return true;
    }
    else if (strncmp("ndscale", wurds[3], 7) == 0)
    {
        printf("Minisynth change Note Number to Decay Scaling!\n");
        int val = atoi(wurds[4]);
        minisynth_set_note_to_decay_scaling(ms, val);
        return true;
    }
    else if (strncmp("noisedb", wurds[3], 7) == 0)
    {
        printf("Minisynth change Noise Osc DB!\n");
        double val = atof(wurds[4]);
        minisynth_set_noise_osc_db(ms, val);
        return true;
    }
    else if (strncmp("oct", wurds[3], 3) == 0)
    {
        printf("Minisynth change OCTAVE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_octave(ms, val);
        return true;
    }
    else if (strncmp("pitchrange", wurds[3], 10) == 0)
    {
        printf("Minisynth change Pitchbend RANGE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_pitchbend_range(ms, val);
        return true;
    }
    else if (strncmp("porta", wurds[3], 5) == 0)
    {
        printf("Minisynth change PORTAMENTO Time!\n");
        double val = atof(wurds[4]);
        minisynth_set_portamento_time_ms(ms, val);
        return true;
    }
    else if (strncmp("pw", wurds[3], 2) == 0)
    {
        printf("Minisynth change PULSEWIDTH Pct!\n");
        double val = atof(wurds[4]);
        minisynth_set_pulsewidth_pct(ms, val);
        return true;
    }
    else if (strncmp("subosc", wurds[3], 6) == 0)
    {
        printf("Minisynth change SubOSC DB!\n");
        double val = atof(wurds[4]);
        minisynth_set_sub_osc_db(ms, val);
        return true;
    }
    else if (strncmp("sustainlvl", wurds[3], 10) == 0)
    {
        double val = atof(wurds[4]);
        printf("Minisynth change Sustain Level! %.2f\n", val);
        minisynth_set_sustain(ms, val);
        return true;
    }
    else if (strncmp("sustainms", wurds[3], 9) == 0)
    {
        printf("Minisynth change Sustain Time ms!\n");
        double val = atof(wurds[4]);
        minisynth_set_sustain_time_ms(ms, val);
        return true;
    }
    else if (strncmp("sustain16th", wurds[3], 11) == 0)
    {
        printf("Minisynth change Sustain Time 16th!\n");
        double val = atof(wurds[4]);
        minisynth_set_sustain_time_sixteenth(ms, val);
        return true;
    }
    else if (strncmp("sustain", wurds[3], 7) == 0)
    {
        printf("Minisynth change SUSTAIN OVERRIDE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_sustain_override(ms, val);
        return true;
    }
    else if (strncmp("vascale", wurds[3], 7) == 0)
    {
        printf("Minisynth change Velocity to Attack Scaling!\n");
        int val = atoi(wurds[4]);
        minisynth_set_velocity_to_attack_scaling(ms, val);
        return true;
    }
    else if (strncmp("voice", wurds[3], 5) == 0)
    {
        printf("Minisynth change VOICE!\n");
        int val = atoi(wurds[4]);
        minisynth_set_voice_mode(ms, val);
        return true;
    }
    else if (strncmp("vol", wurds[3], 3) == 0)
    {
        printf("Minisynth change VOLUME!\n");
        double val = atof(wurds[4]);
        minisynth_set_vol(ms, val);
        return true;
    }
    else if (strncmp("zero", wurds[3], 4) == 0)
    {
        printf("Minisynth change REST-To-ZERO!\n");
        int val = atoi(wurds[4]);
        minisynth_set_reset_to_zero(ms, val);
        return true;
    }
    else if (strncmp("arplatch", wurds[3], 8) == 0)
    {
        printf("Minisynth change ARP Latch!\n");
        int val = atoi(wurds[4]);
        if (val == 0 || val == 1)
            minisynth_set_arpeggiate_latch(ms, val);
        else
            printf("Gimme a 0 or 1\n");
        return true;
    }
    else if (strncmp("arprepeat", wurds[3], 9) == 0)
    {
        printf("Minisynth change ARP Repeat!\n");
        int val = atoi(wurds[4]);
        if (val == 0 || val == 1)
            minisynth_set_arpeggiate_single_note_repeat(ms, val);
        else
            printf("Gimme a 0 or 1\n");
        return true;
    }
    else if (strncmp("arpoctrange", wurds[3], 11) == 0)
    {
        printf("Minisynth change ARP Oct Range!\n");
        int val = atoi(wurds[4]);
        minisynth_set_arpeggiate_octave_range(ms, val);
        return true;
    }
    else if (strncmp("arpmode", wurds[3], 7) == 0)
    {
        printf("Minisynth change ARP Mode!\n");
        int val = atoi(wurds[4]);
        minisynth_set_arpeggiate_mode(ms, val);
        return true;
    }
    else if (strncmp("arprate", wurds[3], 7) == 0)
    {
        printf("Minisynth change ARP Rate!\n");
        int val = atoi(wurds[4]);
        minisynth_set_arpeggiate_rate(ms, val);
        return true;
    }
    else if (strncmp("arpcurstep", wurds[3], 11) == 0)
    {
        printf("you don't change Minisynth curstep - it increments by "
               "itself!\n");
        return true;
    }
    else if (strncmp("arp", wurds[3], 3) == 0)
    {
        printf("Minisynth change ARP!\n");
        int val = atoi(wurds[4]);
        if (val == 0 || val == 1)
            minisynth_set_arpeggiate(ms, val);
        else
            printf("Gimme a 1 or 0\n");
        return true;
    }
    return false;
}

void char_melody_to_midi_melody(synthbase *base, int dest_melody,
                                char char_array[NUM_WURDS][SIZE_OF_WURD],
                                int start, int end)
{
    for (int i = start; i < end; i++)
    {
        int tick = 0;
        int midi_note = 0;
        char char_note[3] = {0};
        bool chord_found = false;
        chord_midi_notes chnotes = {0, 0, 0};
        sscanf(char_array[i], "%d:%s", &tick, char_note);
        if (strncasecmp(char_note, "c", 1) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("C_MAJOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "dm", 2) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("D_MINOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "d", 1) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("D_MAJOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "em", 2) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("E_MINOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "e", 1) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("E_MAJOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "f", 1) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("F_MAJOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "gm", 2) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("G_MINOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "g", 1) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("G_MAJOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "am", 2) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("A_MINOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "a", 1) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("A_MAJOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "bm", 2) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("B_MINOR");
            chord_found = true;
        }
        else if (strncasecmp(char_note, "b", 1) == 0)
        {
            chnotes = get_midi_notes_from_char_chord("B_MAJOR");
            chord_found = true;
        }
        else
        {
            sscanf(char_array[i], "%d:%d", &tick, &midi_note);
            if (midi_note != 0)
            {
                printf("Adding %d:%d\n", tick, midi_note);
                synthbase_add_note(base, dest_melody, tick, midi_note);
            }
        }

        if (chord_found)
        {
            synthbase_add_note(base, dest_melody, tick, chnotes.root);
            synthbase_add_note(base, dest_melody, tick, chnotes.third);
            synthbase_add_note(base, dest_melody, tick, chnotes.fifth);
        }
    }
}
