#include <stdlib.h>
#include <string.h>

#include <cmdloop.h>
#include <digisynth.h>
#include <drumsampler.h>
#include <drumsynth.h>
#include <looper.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <utils.h>

extern mixer *mixr;

bool parse_new_item_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("new", wurds[0], 3) == 0)
    {
        if (strncmp("bitshift", wurds[1], 4) == 0)
        {
            printf("BITSHIFT! PATTERN GEN!\n");
            mixer_add_bitshift(mixr, num_wurds - 2, &wurds[2]);
        }

        else if (strncmp("euclid", wurds[1], 6) == 0)
        {
            int num_hits = atoi(wurds[2]);
            int num_steps = atoi(wurds[3]);
            printf("EUCLIDEAN! PATTERN GEN num_hits:%d num_steps:%d!\n",
                   num_hits, num_steps);
            mixer_add_euclidean(mixr, 3, 16);
        }

        else if (strncmp("digi", wurds[1], 4) == 0 ||
                 strncmp("dsynth", wurds[1], 6) == 0 ||
                 strncmp("ssynth", wurds[1], 6) == 0)
        {
            if (strlen(wurds[2]) != 0)
            {
                int sgnum = add_digisynth(mixr, wurds[2]);
                if (sgnum != -1)
                {
                    soundgenerator *sg = mixr->sound_generators[sgnum];
                    check_and_set_pattern(sg, 0, NOTE_PATTERN, &wurds[3],
                                          num_wurds - 3);
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
            if (sgnum != -1)
            {
                soundgenerator *sg = mixr->sound_generators[sgnum];
                check_and_set_pattern(sg, 0, NOTE_PATTERN, &wurds[2],
                                      num_wurds - 2);
            }
        }

        else if (strncmp("kit", wurds[1], 3) == 0)
        {
            char kickfile[512] = {0};
            get_random_sample_from_dir("kicks", kickfile);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, kickfile);
            drumsampler *bd = new_drumsampler(kickfile);
            int bdnum = add_sound_generator(mixr, (soundgenerator *)bd);
            update_environment("bd", bdnum);

            char snarefile[512] = {0};
            get_random_sample_from_dir("snrs", snarefile);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, snarefile);
            drumsampler *sd = new_drumsampler(snarefile);
            int sdnum = add_sound_generator(mixr, (soundgenerator *)sd);
            update_environment("sd", sdnum);

            char highhat[512] = {0};
            get_random_sample_from_dir("hats", highhat);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, highhat);
            drumsampler *hh = new_drumsampler(highhat);
            int hhnum = add_sound_generator(mixr, (soundgenerator *)hh);
            update_environment("hh", hhnum);

            char perc[512] = {0};
            get_random_sample_from_dir("perc", perc);
            printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, perc);
            drumsampler *pc = new_drumsampler(perc);
            int pcnum = add_sound_generator(mixr, (soundgenerator *)pc);
            update_environment("pc", pcnum);
        }
        else if (strncmp("perc", wurds[1], 4) == 0)
        {
            for (int i = 0; i < 4; i++)
            {
                char perc[512] = {0};
                get_random_sample_from_dir("perc", perc);
                printf(ANSI_COLOR_WHITE "Opening %s\n" ANSI_COLOR_RESET, perc);
                drumsampler *s = new_drumsampler(perc);
                add_sound_generator(mixr, (soundgenerator *)s);
            }
        }

        else if (strncmp("looper", wurds[1], 6) == 0 ||
                 strncmp("loop", wurds[1], 4) == 0 ||
                 strncmp("gran", wurds[1], 4) == 0 ||
                 strncmp("dloop", wurds[1], 5) == 0)
        {
            if (is_valid_file(wurds[2]) || strncmp(wurds[2], "none", 4) == 0)
            {
                double loop_len = atof(wurds[3]);
                if (!loop_len)
                    loop_len = 1;

                int soundgen_num = add_looper(mixr, wurds[2]);
                looper *g = (looper *)mixr->sound_generators[soundgen_num];

                if (strncmp("gran", wurds[1], 4) == 0)
                    looper_set_loop_mode(g, LOOPER_SMUDGE_MODE);
                else
                    looper_set_loop_mode(g, LOOPER_LOOP_MODE);

                if (strncmp("dloop", wurds[1], 5) == 0)
                    looper_set_grain_density(g, 100);

                looper_set_loop_len(g, loop_len);
            }
        }

        if (strncmp("markov", wurds[1], 6) == 0)
        {
            printf("MARkOV! PATTERN GEN!\n");
            unsigned int type = atoi(wurds[2]);
            mixer_add_markov(mixr, type);
        }

        else if (strncmp("moog", wurds[1], 4) == 0)
        {
            int sgnum = add_minisynth(mixr);
            if (sgnum != -1)
            {
                soundgenerator *sg = mixr->sound_generators[sgnum];
                check_and_set_pattern(sg, 0, NOTE_PATTERN, &wurds[2],
                                      num_wurds - 2);
            }
        }

        else if (strncmp("samp", wurds[1], 4) == 0 ||
                 strncmp("step", wurds[1], 4) == 0)
        {
            int sgnum = -1;
            if (strlen(wurds[2]) == 0 || strncmp("synth", wurds[2], 5) == 0)
            {
                drumsynth *ds = new_drumsynth();
                sgnum = add_sound_generator(mixr, (soundgenerator *)ds);
            }
            else if (is_valid_file(wurds[2]))
            {
                drumsampler *s = new_drumsampler(wurds[2]);
                sgnum = add_sound_generator(mixr, (soundgenerator *)s);
            }
            if (sgnum != -1)
            {
                soundgenerator *sg = mixr->sound_generators[sgnum];
                check_and_set_pattern(sg, 0, STEP_PATTERN, &wurds[3],
                                      num_wurds - 3);
            }
        }

        return true;
    }
    return false;
}
