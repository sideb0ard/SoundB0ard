#include <stdlib.h>
#include <string.h>

#include <bitshift.h>
#include <euclidean.h>
#include <markov.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <sequence_generator_cmds.h>
#include <sequencer_utils.h>

extern mixer *mixr;

bool parse_sequence_generator_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("sequence", wurds[0], 7) == 0 ||
        strncmp("seq", wurds[0], 3) == 0)
    {
        int sgnum = atoi(wurds[1]);
        if (mixer_is_valid_seq_gen_num(mixr, sgnum))
        {
            sequence_generator *sg = mixr->sequence_generators[sgnum];

            if (strncmp("debug", wurds[2], 5) == 0)
            {
                printf("Enabling DEBUG on sequence GEN %d\n", sgnum);
                int enable = atoi(wurds[3]);
                sg->set_debug(sg, enable);
            }
            else if (strncmp("gen", wurds[2], 3) == 0)
            {
                int num =
                    sg->generate(sg, (void *)&mixr->timing_info.cur_sample);
                char binnum[17] = {0};
                char_binary_version_of_short(num, binnum);
                printf("NOM!: %d %s\n", num, binnum);
            }
            else if (strncmp("time", wurds[2], 4) == 0 && sg->type == BITSHIFT)
            {
                int itime = atoi(wurds[3]);
                bitshift *bs = (bitshift *)sg;
                bitshift_set_time_counter(bs, itime);
            }
            else if (sg->type == EUCLIDEAN)
            {
                euclidean *e = (euclidean *)sg;
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
            else if (sg->type == MARKOV)
            {
                markov *m = (markov *)sg;
                if (strncmp(wurds[2], "type", 4) == 0)
                {
                    int type = atoi(wurds[3]);
                    markov_set_type(m, type);
                }
            }
        }
        return true;
    }
    else if (strncmp("beat", wurds[0], 4) == 0 ||
             strncmp("note", wurds[0], 4) == 0)
    {
        int sg_num;
        int sg_pattern_num;
        sscanf(wurds[1], "%d:%d", &sg_num, &sg_pattern_num);
        if (mixer_is_valid_soundgen_num(mixr, sg_num))
        {

            int line_len = 0;
            for (int i = 2; i < num_wurds; i++)
            {
                line_len += strlen(wurds[i]);
            }
            line_len += num_wurds + 1;
            char line[line_len];
            memset(line, 0, line_len * sizeof(char));

            for (int i = 2; i < num_wurds; i++)
            {
                strcat(line, wurds[i]);
                if (i != num_wurds - 1)
                    strcat(line, " ");
            }

            midi_event *pattern = calloc(PPBAR, sizeof(midi_event));
            int pattern_type = -1;
            if (strncmp("beat", wurds[0], 4) == 0)
                pattern_type = STEP_PATTERN;
            else
                pattern_type = NOTE_PATTERN;
            if (parse_pattern(line, pattern, pattern_type))
            {
                soundgenerator *sg = mixr->sound_generators[sg_num];
                int num_patterns = sg->get_num_patterns(sg);
                printf("NUM PAYYTERSN! %d\n", num_patterns);
                if (num_patterns <= sg_pattern_num)
                    sg->set_num_patterns(sg, sg_pattern_num + 1);
                sg->set_pattern(sg, sg_pattern_num, pattern);
            }
            free(pattern);
            return true;
        }
    }
    return false;
}
