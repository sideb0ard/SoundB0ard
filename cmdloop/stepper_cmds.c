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
#include "chaosmonkey.h"
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

bool parse_stepper_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("samp", wurds[0], 3) == 0)
    {

        char *pattern = (char *)calloc(151, sizeof(char));

        int soundgen_num = atoi(wurds[1]);
        if (mixer_is_valid_soundgen_num(mixr, soundgen_num) &&
            mixr->sound_generators[soundgen_num]->type == SEQUENCER_TYPE)
        {

            sample_sequencer *s =
                (sample_sequencer *)mixr->sound_generators[soundgen_num];

            if (strncmp("load", wurds[2], 4) == 0 ||
                strncmp("import", wurds[2], 6) == 0)
            {
                if (is_valid_file(wurds[3]))
                {
                    printf("Changing Loaded "
                           "FILE!\n");
                    sample_seq_import_file(s, wurds[3]);
                }
                else
                    printf("%s is not a valid file\n", wurds[3]);
            }
            else if (strncmp("morph", wurds[2], 8) == 0)
            {
                printf("M0RRRPH!\n");
                s->morph = 1 - s->morph;
            }
            else if (strncmp("pitch", wurds[2], 5) == 0)
            {
                printf("PITCHHHHy!!\n");
                double v = atof(wurds[3]);
                sample_sequencer_set_pitch(s, v);
            }
            else
            {
                sequencer *seq = &s->m_seq;
                parse_sample_sequencer_command(seq, wurds, num_wurds, pattern);
            }
        }
        free(pattern);
        return true;
    }
    return false;
}

void parse_sample_sequencer_command(sequencer *seq, char wurds[][SIZE_OF_WURD],
                                    int num_wurds, char *pattern)
{
    if (strncmp("add", wurds[2], 3) == 0)
    {
        printf("Adding\n");
        char_array_to_seq_string_pattern(seq, pattern, wurds, 3, num_wurds);
        add_char_pattern(seq, pattern);
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
    else if (strncmp("sloppy", wurds[2], 6) == 0)
    {
        int sloppyjoe = atoi(wurds[3]);
        seq_set_sloppiness(seq, sloppyjoe);
    }
    else if (strncmp("generate", wurds[2], 8) == 0 ||
             strncmp("gen", wurds[2], 3) == 0)
    {
        if (strncmp("every", wurds[3], 5) == 0)
        {
            int num_gens = atoi(wurds[4]);
            if (num_gens > 0)
            {
                seq_set_generate_mode(seq, true);
                seq->generate_every_n_loops = num_gens;
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
                seq_set_generate_mode(seq, true);
                seq_set_max_generations(seq, num_gens);
            }
            else
            {
                printf("Need a number for 'for'\n");
            }
        }
        else if (strncmp("source", wurds[3], 6) == 0 ||
                 strncmp("src", wurds[3], 3) == 0)
        {
            int generate_src = atoi(wurds[4]);
            if (mixer_is_valid_seq_gen_num(mixr, generate_src))
            {
                seq_set_generate_src(seq, generate_src);
            }
            else
                printf("not a valid generate SRC: %d\n", generate_src);
        }
        else
        {
            seq_set_generate_mode(seq, 1 - seq->generate_mode);
        }
    }
    else if (strncmp("pattern_len", wurds[2], 10) == 0)
    {
        int len = atoi(wurds[3]);
        seq_set_pattern_len(seq, len);
    }
    else if (strncmp("visualize", wurds[2], 9) == 0)
    {
        bool b = atoi(wurds[3]);
        printf("Setting visualize to %s\n", b ? "true" : "false");
        seq->visualize = b;
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
