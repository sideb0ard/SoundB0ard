#include <stdlib.h>
#include <string.h>

#include <cmdloop/pattern_generator_cmds.h>
#include <mixer.h>
#include <pattern_generators/recursive_pattern_gen.h>
#include <pattern_parser.h>

extern mixer *mixr;

bool parse_pattern_generator_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
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
            // else if (strncmp("gen", wurds[2], 3) == 0)
            //{
            //    int num =
            //        sg->generate(sg, (void *)&mixr->timing_info.cur_sample);
            //    char binnum[17] = {0};
            //    char_binary_version_of_short(num, binnum);
            //    printf("NOM!: %d %s\n", num, binnum);
            //}
            else if (pg->type == RECURSIVE_PATTERN_TYPE)
            {
                recursive_pattern_gen *rpg = (recursive_pattern_gen *)pg;
                float val = atof(wurds[3]);
                if (strncmp("thresh", wurds[2], 5) == 0)
                {
                    recursive_pattern_gen_set_thresh(rpg, val);
                }
                else if (strncmp("divisor", wurds[2], 8) == 0)
                {
                    recursive_pattern_gen_set_divisor(rpg, val);
                }
                else if (strncmp("multi", wurds[2], 5) == 0)
                {
                    recursive_pattern_gen_set_multi(rpg, val);
                }
            }
        }
        return true;
    }
    return false;
}
