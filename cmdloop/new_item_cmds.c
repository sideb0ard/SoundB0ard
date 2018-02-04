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

extern mixer *mixr;

bool parse_new_item_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("new", wurds[0], 3) == 0)
    {
        if (strncmp("algo", wurds[1], 4) == 0)
        {
            printf("NUM WURDS!%d\n", num_wurds);
            for (int i = 0; i < num_wurds; i++)
                printf("wurd: %s\n", wurds[i]);
            add_algorithm(num_wurds - 2, &wurds[2]);
        }

        else if (strncmp("bitshift", wurds[1], 4) == 0)
        {
            printf("BITSHIFT! SEQUENCE GEN!\n");
            mixer_add_bitshift(mixr, num_wurds - 2, &wurds[2]);
        }

        else if (strncmp("euclid", wurds[1], 6) == 0)
        {
            if (num_wurds != 4)
            {
                printf("get a life, mate - need to gimme number of hits "
                       "and number of steps\n");
            }
            else
            {
                int num_hits = atoi(wurds[2]);
                int num_steps = atoi(wurds[3]);
                printf("EUCLIDEAN! SEQUENCE GEN num_hits:%d num_steps:%d!\n",
                       num_hits, num_steps);
                mixer_add_euclidean(mixr, num_hits, num_steps);
            }
        }

        else if (strncmp("digi", wurds[1], 4) == 0 ||
                 strncmp("dsynth", wurds[1], 6) == 0 ||
                 strncmp("ssynth", wurds[1], 6) == 0)
        {
            if (strlen(wurds[2]) != 0)
            {
                int sgnum = add_digisynth(mixr, wurds[2]);
                mixr->midi_control_destination = SYNTH;
                mixr->active_midi_soundgen_num = sgnum;
                if (num_wurds > 2)
                {
                    digisynth *ds = (digisynth *)mixr->sound_generators[sgnum];
                    char_melody_to_midi_melody(&ds->base, 0, wurds, 2,
                                               num_wurds);
                }
            }
            else
            {
                printf("Need to give me a sample name for a digisynth..\n");
            }
        }

        else if (strncmp("fm", wurds[1], 2) == 0 ||
                 strncmp("dx", wurds[1], 2) == 0)
        {
            int sgnum = add_dxsynth(mixr);
            mixr->midi_control_destination = SYNTH;
            mixr->active_midi_soundgen_num = sgnum;
            if (num_wurds > 2)
            {
                dxsynth *dx = (dxsynth *)mixr->sound_generators[sgnum];
                char_melody_to_midi_melody(&dx->base, 0, wurds, 2, num_wurds);
            }
        }

        else if (strncmp("kit", wurds[1], 3) == 0)
        {
            char *pattern = (char *)calloc(151, sizeof(char));
            memset(pattern, 0, 151);

            char kickfile[512] = {0};
            get_random_sample_from_dir("kicks", kickfile);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, kickfile);
            sample_sequencer *bd = new_sample_seq(kickfile);
            int bdnum = add_sound_generator(mixr, (soundgenerator *)bd);
            pattern_char_to_pattern(
                &bd->m_seq, pattern,
                bd->m_seq.patterns[bd->m_seq.num_patterns++]);
            update_environment("bd", bdnum);

            char snarefile[512] = {0};
            get_random_sample_from_dir("snrs", snarefile);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, snarefile);
            sample_sequencer *sd = new_sample_seq(snarefile);
            int sdnum = add_sound_generator(mixr, (soundgenerator *)sd);
            pattern_char_to_pattern(
                &sd->m_seq, pattern,
                sd->m_seq.patterns[sd->m_seq.num_patterns++]);
            update_environment("sd", sdnum);

            char highhat[512] = {0};
            get_random_sample_from_dir("hats", highhat);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, highhat);
            sample_sequencer *hh = new_sample_seq(highhat);
            int hhnum = add_sound_generator(mixr, (soundgenerator *)hh);
            pattern_char_to_pattern(
                &hh->m_seq, pattern,
                hh->m_seq.patterns[hh->m_seq.num_patterns++]);
            update_environment("hh", hhnum);

            char perc[512] = {0};
            get_random_sample_from_dir("perc", perc);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, perc);
            sample_sequencer *pc = new_sample_seq(perc);
            int pcnum = add_sound_generator(mixr, (soundgenerator *)pc);
            pattern_char_to_pattern(
                &pc->m_seq, pattern,
                pc->m_seq.patterns[pc->m_seq.num_patterns++]);
            update_environment("pc", pcnum);

            free(pattern);
        }

        else if (strncmp("looper", wurds[1], 6) == 0 ||
                 strncmp("loop", wurds[1], 4) == 0 ||
                 strncmp("gran", wurds[1], 4) == 0)
        {
            if (is_valid_file(wurds[2]) || strncmp(wurds[2], "none", 4) == 0)
            {
                double loop_len = atof(wurds[3]);
                if (!loop_len)
                    loop_len = 1;

                int soundgen_num = add_looper(mixr, wurds[2]);
                looper *g = (looper *)mixr->sound_generators[soundgen_num];

                if (strncmp("gran", wurds[1], 4) == 0)
                    looper_set_granulate_mode(g, true);
                else
                    looper_set_loop_mode(g, true);

                looper_set_loop_len(g, loop_len);
            }
        }

        else if (strncmp("moog", wurds[1], 4) == 0)
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

        else if (strncmp("samp", wurds[1], 4) == 0)
        {
            char *pattern = (char *)calloc(151, sizeof(char));
            memset(pattern, 0, 151);
            if (is_valid_file(wurds[2]))
            {
                sample_sequencer *s = new_sample_seq(wurds[2]);
                char_array_to_seq_string_pattern(&s->m_seq, pattern, wurds, 3,
                                                 num_wurds);
                int sgnum = add_sound_generator(
                    mixr,
                    (soundgenerator *)s); //  add_seq_char_pattern(mixr,
                                          //  wurds[1], pattern);
                pattern_char_to_pattern(
                    &s->m_seq, pattern,
                    s->m_seq.patterns[s->m_seq.num_patterns++]);

                printf("New SG at pos %d - has %d patterns\n", sgnum,
                       s->m_seq.num_patterns);
            }
            free(pattern);
        }

        return true;
    }
    return false;
}
