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
#include <stepper_cmds.h>
#include <synth_cmds.h>

extern mixer *mixr;
bool parse_looper_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("looper", wurds[0], 8) == 0 ||
        strncmp("gran", wurds[0], 4) == 0 || strncmp("loop", wurds[0], 4) == 0)
    {
        if (is_valid_file(wurds[1]) || strncmp(wurds[1], "none", 4) == 0)
        {
            printf("VALID!\n");
            int soundgen_num = add_looper(mixr, wurds[1]);
            printf("soundgenerator %d\n", soundgen_num);
        }
        else
        {
            int soundgen_num = atoi(wurds[1]);
            if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
                mixr->sound_generators[soundgen_num]->type == LOOPER_TYPE)
            {
                looper *g = (looper *)mixr->sound_generators[soundgen_num];

                if (strncmp("grain_duration_ms", wurds[2], 14) == 0)
                {
                    int dur = atoi(wurds[3]);
                    looper_set_grain_duration(g, dur);
                }
                else if (strncmp("grains_per_sec", wurds[2], 14) == 0)
                {
                    int gps = atoi(wurds[3]);
                    looper_set_grains_per_sec(g, gps);
                }
                else if (strncmp("audio_buffer_read_idx", wurds[2], 14) == 0)
                {
                    int pos = atoi(wurds[3]);
                    looper_set_audio_buffer_read_idx(g, pos);
                }
                else if (strncmp("grain_spray_ms", wurds[2], 14) == 0)
                {
                    int spray = atoi(wurds[3]);
                    looper_set_granular_spray(g, spray);
                }
                else if (strncmp("quasi_grain_fudge", wurds[2], 17) == 0)
                {
                    int fudge = atoi(wurds[3]);
                    looper_set_quasi_grain_fudge(g, fudge);
                }
                else if (strncmp("extsource", wurds[2], 9) == 0)
                {
                    printf("EXTSOURCE!\n");
                    int sg = atoi(wurds[3]);
                    if (mixer_is_valid_soundgen_num(mixr, sg))
                    {
                        printf("GRAN is following "
                               "%d\n",
                               sg);
                        looper_set_external_source(g, sg);
                    }
                }
                else if (strncmp("file", wurds[2], 4) == 0 ||
                         strncmp("open", wurds[2], 4) == 0 ||
                         strncmp("import", wurds[2], 6) == 0)
                {
                    if (is_valid_file(wurds[3]))
                    {
                        looper_import_file(g, wurds[3]);
                    }
                }
                else if (strncmp("grain_pitch", wurds[2], 10) == 0)
                {
                    double pitch = atof(wurds[3]);
                    looper_set_grain_pitch(g, pitch);
                }
                else if (strncmp("selection_mode", wurds[2], 14) == 0)
                {
                    int mode = atoi(wurds[3]);
                    looper_set_selection_mode(g, mode);
                }
                else if (strncmp("env_mode", wurds[2], 8) == 0)
                {
                    int mode = atoi(wurds[3]);
                    printf("ENV MODE is %d\n", mode);
                    looper_set_envelope_mode(g, mode);
                }
                else if (strncmp("loop_mode", wurds[2], 9) == 0)
                {
                    int mode = atoi(wurds[3]);
                    printf("loop MODE is %d\n", mode);
                    looper_set_loop_mode(g, mode);
                }
                else if (strncmp("loop_len", wurds[2], 8) == 0)
                {
                    double len = atof(wurds[3]);
                    printf("loop LEN is %f\n", len);
                    looper_set_loop_len(g, len);
                }
                else if (strncmp("scramble", wurds[2], 8) == 0)
                {
                    bool b = atoi(wurds[3]);
                    looper_set_scramble_mode(g, b);
                }
                else if (strncmp("sequencer_mode", wurds[2], 13) == 0)
                {
                    int mode = atoi(wurds[3]);
                    printf("MODE is %d\n", mode);
                    looper_set_sequencer_mode(g, mode);
                }
                else if (strncmp("stutter", wurds[2], 8) == 0)
                {
                    bool b = atoi(wurds[3]);
                    looper_set_stutter_mode(g, b);
                }
                else if (strncmp("movement", wurds[2], 8) == 0)
                {
                    int mode = atoi(wurds[3]);
                    printf("Movement MODE is %d\n", mode);
                    looper_set_movement_mode(g, mode);
                }
                else if (strncmp("reverse", wurds[2], 8) == 0)
                {
                    int mode = atoi(wurds[3]);
                    printf("REverse MODE is %d\n", mode);
                    looper_set_reverse_mode(g, mode);
                }
                else if (strncmp("lfo1_type", wurds[2], 8) == 0)
                {
                    int type = atoi(wurds[3]);
                    printf("LFO TYPE is %d\n", mode);
                    looper_set_lfo_voice(g, 1, type);
                }
                else if (strncmp("lfo1_amp", wurds[2], 7) == 0)
                {
                    double amp = atof(wurds[3]);
                    looper_set_lfo_amp(g, 1, amp);
                }
                else if (strncmp("lfo1_rate", wurds[2], 8) == 0)
                {
                    double rate = atof(wurds[3]);
                    looper_set_lfo_rate(g, 1, rate);
                }
                else if (strncmp("lfo1_sync", wurds[2], 8) == 0)
                {
                    double loops = atof(wurds[3]);
                    looper_set_lfo_sync(g, 1, loops);
                }
                else if (strncmp("lfo1_min", wurds[2], 8) == 0)
                {
                    double min = atof(wurds[3]);
                    looper_set_lfo_min(g, 1, min);
                }
                else if (strncmp("lfo1_max", wurds[2], 8) == 0)
                {
                    double max = atof(wurds[3]);
                    looper_set_lfo_max(g, 1, max);
                }
                else if (strncmp("lfo2_type", wurds[2], 8) == 0)
                {
                    int type = atoi(wurds[3]);
                    printf("LFO TYPE is %d\n", mode);
                    looper_set_lfo_voice(g, 2, type);
                }
                else if (strncmp("lfo2_amp", wurds[2], 7) == 0)
                {
                    double amp = atof(wurds[3]);
                    looper_set_lfo_amp(g, 2, amp);
                }
                else if (strncmp("lfo2_rate", wurds[2], 8) == 0)
                {
                    double rate = atof(wurds[3]);
                    looper_set_lfo_rate(g, 2, rate);
                }
                else if (strncmp("lfo2_sync", wurds[2], 8) == 0)
                {
                    double loops = atof(wurds[3]);
                    looper_set_lfo_sync(g, 2, loops);
                }
                else if (strncmp("lfo2_min", wurds[2], 8) == 0)
                {
                    double min = atof(wurds[3]);
                    looper_set_lfo_min(g, 2, min);
                }
                else if (strncmp("lfo2_max", wurds[2], 8) == 0)
                {
                    double max = atof(wurds[3]);
                    looper_set_lfo_max(g, 2, max);
                }
                else if (strncmp("lfo3_type", wurds[2], 8) == 0)
                {
                    int type = atoi(wurds[3]);
                    printf("LFO TYPE is %d\n", mode);
                    looper_set_lfo_voice(g, 3, type);
                }
                else if (strncmp("lfo3_amp", wurds[2], 7) == 0)
                {
                    double amp = atof(wurds[3]);
                    looper_set_lfo_amp(g, 3, amp);
                }
                else if (strncmp("lfo3_rate", wurds[2], 8) == 0)
                {
                    double rate = atof(wurds[3]);
                    looper_set_lfo_rate(g, 3, rate);
                }
                else if (strncmp("lfo3_sync", wurds[2], 8) == 0)
                {
                    double loops = atof(wurds[3]);
                    looper_set_lfo_sync(g, 3, loops);
                }
                else if (strncmp("lfo3_min", wurds[2], 8) == 0)
                {
                    double min = atof(wurds[3]);
                    looper_set_lfo_min(g, 3, min);
                }
                else if (strncmp("lfo3_max", wurds[2], 8) == 0)
                {
                    double max = atof(wurds[3]);
                    looper_set_lfo_max(g, 3, max);
                }
                else if (strncmp("lfo4_type", wurds[2], 8) == 0)
                {
                    int type = atoi(wurds[3]);
                    printf("LFO TYPE is %d\n", mode);
                    looper_set_lfo_voice(g, 4, type);
                }
                else if (strncmp("lfo4_amp", wurds[2], 7) == 0)
                {
                    double amp = atof(wurds[3]);
                    looper_set_lfo_amp(g, 4, amp);
                }
                else if (strncmp("lfo4_rate", wurds[2], 8) == 0)
                {
                    double rate = atof(wurds[3]);
                    looper_set_lfo_rate(g, 4, rate);
                }
                else if (strncmp("lfo4_sync", wurds[2], 8) == 0)
                {
                    double loops = atof(wurds[3]);
                    looper_set_lfo_sync(g, 4, loops);
                }
                else if (strncmp("lfo4_min", wurds[2], 8) == 0)
                {
                    double min = atof(wurds[3]);
                    looper_set_lfo_min(g, 4, min);
                }
                else if (strncmp("lfo4_max", wurds[2], 8) == 0)
                {
                    double max = atof(wurds[3]);
                    looper_set_lfo_max(g, 4, max);
                }
                else if (strncmp("graindur_lfo_on", wurds[2], 14) == 0)
                {
                    int b = atoi(wurds[3]);
                    g->graindur_lfo_on = b;
                }
                else if (strncmp("grainpitch_lfo_on", wurds[2], 17) == 0)
                {
                    int b = atoi(wurds[3]);
                    g->grainpitch_lfo_on = b;
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
                else
                {
                    printf("ELSEY SEQUENCE!\n");
                    char *pattern = (char *)calloc(151, sizeof(char));
                    sequencer *seq = &g->m_seq;
                    parse_sample_sequencer_command(seq, wurds, num_wurds,
                                                   pattern);
                    free(pattern);
                }
            }
        }
        return true;
    }
    return false;
}
