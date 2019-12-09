#include <stdlib.h>
#include <string.h>

#include <bitshift.h>
#include <euclidean.h>
#include <intdiv.h>
#include <juggler.h>
#include <markov.h>
#include <mixer.h>
#include <pattern_generator_cmds.h>
#include <pattern_parser.h>
#include <pattern_utils.h>

extern mixer *mixr;

bool parse_pattern_generator_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    (void)num_wurds;
    if (strncmp("pattern", wurds[0], 7) == 0 ||
        strncmp("pat", wurds[0], 3) == 0)
    {
        int pgnum = atoi(wurds[1]);
        if (mixer_is_valid_pattern_gen_num(mixr, pgnum))
        {
            pattern_generator *pg = mixr->pattern_generators[pgnum];

            if (strncmp("debug", wurds[2], 5) == 0)
            {
                printf("Enabling DEBUG on pattern GEN %d\n", pgnum);
                int enable = atoi(wurds[3]);
                pg->set_debug(pg, enable);
            }
            else if (strncmp("gen", wurds[2], 3) == 0)
            {
                midi_event midi_pattern[PPBAR] = {};
                pg->generate(pg, &midi_pattern);
                uint16_t bit_pattern = midi_pattern_to_short(midi_pattern);
                char binnum[17] = {0};
                short_to_char(bit_pattern, binnum);
                printf("NOM!: %d %s\n", bit_pattern, binnum);
            }
            else if (strncmp("time", wurds[2], 4) == 0 && pg->type == BITSHIFT)
            {
                int itime = atoi(wurds[3]);
                bitshift *bs = (bitshift *)pg;
                bitshift_set_time_counter(bs, itime);
            }
            else if (pg->type == EUCLIDEAN)
            {
                euclidean *e = (euclidean *)pg;
                if (strncmp(wurds[2], "mode", 4) == 0)
                {
                    int mode = atoi(wurds[3]);
                    euclidean_change_mode(e, mode);
                }
                if (strncmp(wurds[2], "hits", 4) == 0)
                {
                    int val = atoi(wurds[3]);
                    euclidean_change_hits(e, val);
                }
                if (strncmp(wurds[2], "steps", 5) == 0)
                {
                    int val = atoi(wurds[3]);
                    euclidean_change_steps(e, val);
                }
            }
            else if (pg->type == INTDIV)
            {
                intdiv *id = (intdiv *)pg;
                if (strncmp("print", wurds[2], 5) == 0)
                    intdiv_print_patterns(id);
                else if (strncmp("selected", wurds[2], 8) == 0)
                {
                    int val = atoi(wurds[3]);
                    intdiv_set_selected_pattern(id, val);
                }
                else if (strncmp("mode", wurds[2], 4) == 0)
                {
                    int val = atoi(wurds[3]);
                    intdiv_set_mode(id, val);
                }
            }
            else if (pg->type == JUGGLER)
            {
                juggler *j = (juggler *)pg;
                int val = atoi(wurds[3]);
                if (strncmp(wurds[2], "max_depth", 9) == 0)
                    juggler_set_max_depth(j, val);
                else if (strncmp(wurds[2], "pct_probability", 15) == 0)
                    juggler_set_pct_probability(j, val);
                else if (strncmp(wurds[2], "debug", 5) == 0)
                    juggler_set_debug(j, val);
            }
            else if (pg->type == MARKOV)
            {
                markov *m = (markov *)pg;
                if (strncmp(wurds[2], "type", 4) == 0)
                {
                    int type = atoi(wurds[3]);
                    markov_set_type(m, type);
                }
            }
        }
        return true;
    }
    // else if (strncmp("beat", wurds[0], 4) == 0 ||
    //         strncmp("note", wurds[0], 4) == 0)
    //{
    //    int sg_num;
    //    int sg_pattern_num;
    //    sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
    //    if (mixer_is_valid_soundgen_num(mixr, sg_num))
    //    {

    //        int line_len = 0;
    //        for (int i = 2; i < num_wurds; i++)
    //        {
    //            line_len += strlen(wurds[i]);
    //        }
    //        line_len += num_wurds + 1;
    //        char line[line_len];
    //        memset(line, 0, line_len * sizeof(char));

    //        for (int i = 2; i < num_wurds; i++)
    //        {
    //            strcat(line, wurds[i]);
    //            if (i != num_wurds - 1)
    //                strcat(line, " ");
    //        }

    //        midi_event *pattern = calloc(PPBAR, sizeof(midi_event));
    //        int pattern_type = -1;
    //        if (strncmp("beat", wurds[0], 4) == 0)
    //            pattern_type = STEP_PATTERN;
    //        else
    //            pattern_type = NOTE_PATTERN;
    //        if (parse_pattern(line, pattern, pattern_type))
    //        {
    //            soundgenerator *sg = mixr->sound_generators[sg_num];
    //            int num_patterns = sg->get_num_patterns(sg);
    //            printf("NUM PAYYTERSN! %d\n", num_patterns);
    //            if (num_patterns <= sg_pattern_num)
    //                sg->set_num_patterns(sg, sg_pattern_num + 1);
    //            pattern_change_info change_info = {.clear_previous = true,
    //                                               .temporary = false};
    //            sg->set_pattern(sg, sg_pattern_num, change_info, pattern);
    //        }
    //        free(pattern);
    //        return true;
    //    }
    //}
    return false;
}
