#include <stdlib.h>
#include <string.h>

#include <mixer.h>
#include <value_generator_cmds.h>

extern mixer *mixr;

bool parse_value_generator_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("val", wurds[0], 3) == 0)
    {
        int vgnum = atoi(wurds[1]);
        if (mixer_is_valid_value_gen_num(mixr, vgnum))
        {
            value_generator *vg = mixr->value_generators[vgnum];
            unsigned int value_type = vg->values_type;

            if (strncmp("gen", wurds[2], 3) == 0)
            {

                if (value_type == LIST_VALUE_CHAR_TYPE)
                {
                    char wurd[SIZE_OF_WURD] = {};
                    list_value_holder val = vg->generate(vg);
                    printf("GOTS %s\n", val.wurd);
                }
                else
                {
                    list_value_holder val = vg->generate(vg);
                    printf("GOTS %f\n", val.val);
                }
            }
        }
        return true;
    }
    return false;
}
