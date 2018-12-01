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

        else if (strncmp("juggle", wurds[1], 6) == 0)
        {
            printf("JUGGLER! PATTERN GEN!\n");
            mixer_add_juggler(mixr, 0); // 0 is only choice at the mo
        }

        else if (strncmp("fval", wurds[1], 4) == 0 ||
                 strncmp("cval", wurds[1], 4) == 0)
        {
            void *data;
            int num_items = num_wurds - 2;
            int list_type = -1;
            if (strncmp("fval", wurds[1], 4) == 0)
            {
                list_type = LIST_VALUE_FLOAT_TYPE;
                data = calloc(num_items, sizeof(float));
                float *fdata = (float *)data;
                for (int i = 0, j = 2; i < num_items; i++, j++)
                {
                    fdata[i] = atof(wurds[j]);
                    printf("COPied %f\n", fdata[i]);
                }
            }
            else
            {
                list_type = LIST_VALUE_CHAR_TYPE;
                char **cdata = calloc(num_items, sizeof(char*));
                for (int i = 0; i < num_items; i++)
                    cdata[i] = calloc(1, sizeof(char)*SIZE_OF_WURD);

                for (int i = 0, j = 2; i < num_items; i++, j++)
                    strcpy(cdata[i], wurds[j]);

                data = cdata;
            }

            int vg_num = mixer_add_value_list(
                mixr, list_type, num_items, data); // ** takes ownership of memory **
        }

        else if (strncmp("kit", wurds[1], 3) == 0 ||
                 strncmp("beat", wurds[1], 4) == 0)
        {
            drumsynth *bd = new_drumsynth();
            drumsynth_open_patch(bd, "thuuud");
            int bdnum = add_sound_generator(mixr, (soundgenerator *)bd);
            update_environment("bd", bdnum);

            drumsynth *sd = new_drumsynth();
            drumsynth_open_patch(sd, "snarrre");
            int sdnum = add_sound_generator(mixr, (soundgenerator *)sd);
            update_environment("sd", sdnum);

            drumsynth *hh = new_drumsynth();
            drumsynth_open_patch(hh, "closedhh");
            int hhnum = add_sound_generator(mixr, (soundgenerator *)hh);
            update_environment("hh", hhnum);

            if (strncmp("beat", wurds[1], 4) == 0)
            {
                char launch_cmd[6][SIZE_OF_WURD] = {"every", "2", "bar",
                                                    "apply"};

                int drum_markov = mixer_add_markov(mixr, 0);
                sprintf(launch_cmd[4], "%d", drum_markov);
                sprintf(launch_cmd[5], "%d:0", bdnum);

                algorithm *a = new_algorithm(6, launch_cmd);
                if (a)
                    mixer_add_algorithm(mixr, a);

                int snare_markov = mixer_add_markov(mixr, 4);
                sprintf(launch_cmd[4], "%d", snare_markov);
                sprintf(launch_cmd[5], "%d:0", sdnum);

                a = new_algorithm(6, launch_cmd);
                if (a)
                    mixer_add_algorithm(mixr, a);

                int hats_markov = mixer_add_markov(mixr, 2);
                sprintf(launch_cmd[4], "%d", hats_markov);
                sprintf(launch_cmd[5], "%d:0", hhnum);

                a = new_algorithm(6, launch_cmd);
                if (a)
                    mixer_add_algorithm(mixr, a);
            }
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
            if (strncmp("clap", wurds[2], 4) == 0 ||
                     strncmp("snare", wurds[2], 5) == 0)
                mixer_add_markov(mixr, 0);
            else if (strncmp("garage", wurds[2], 6) == 0 ||
                strncmp("2step", wurds[2], 5) == 0)
                mixer_add_markov(mixr, 1);
            else if (strncmp("hatsmask", wurds[2], 8) == 0)
                mixer_add_markov(mixr, 4);
            else if (strncmp("hats2", wurds[2], 5) == 0)
                mixer_add_markov(mixr, 3);
            else if (strncmp("hats", wurds[2], 4) == 0)
                mixer_add_markov(mixr, 2);
            else if (strncmp("hiphop", wurds[2], 6) == 0)
                mixer_add_markov(mixr, 5);
            else if (strncmp("house", wurds[2], 4) == 0)
                mixer_add_markov(mixr, 6);
            else if (strncmp("raggakick", wurds[2], 9) == 0)
                mixer_add_markov(mixr, 7);
            else if (strncmp("raggasnare", wurds[2], 10) == 0)
                mixer_add_markov(mixr, 8);
            else
            {
                unsigned int type = atoi(wurds[2]);
                mixer_add_markov(mixr, type);
            }
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
