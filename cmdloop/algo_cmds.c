#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algo_cmds.h>
#include <mixer.h>

extern mixer *mixr;

bool parse_algo_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    if (strncmp("algo", wurds[0], 4) == 0)
    {
        printf("ALGO!\n");
        int algo_num = atoi(wurds[1]);
        if (is_valid_algo_num(algo_num))
        {
            printf("valid\n");
            algorithm *a = mixr->algorithms[algo_num];

            if (strncmp("on", wurds[2], 2) == 0 ||
                strncmp("start", wurds[2], 4) == 0)
                a->active = true;
            else if (strncmp("off", wurds[2], 3) == 0 ||
                     strncmp("stop", wurds[2], 4) == 0)
                a->active = false;
            else if (strncmp("step", wurds[2], 4) == 0)
            {
                int step = atoi(wurds[3]);
                a->process_step = step;
            }
            else if (strncmp("event", wurds[2], 5) == 0)
            {
                int event_type = algorithm_get_event_type_from_string(wurds[3]);
                if (event_type != -1)
                    a->event_type = event_type;
                else
                    printf("Nah!\n");
            }
            else if (strncmp("var_select", wurds[2], 10) == 0)
            {
                int var_select_type =
                    algorithm_get_var_select_type_from_string(wurds[3]);
                a->var_select_type = var_select_type;
            }
            else if (strncmp("var_list", wurds[2], 8) == 0)
            {
                printf("VAR_LIST! %s\n", wurds[3]);
            }
            else if (strncmp("cmd", wurds[2], 3) == 0)
            {
                printf("NEW CMD! %s\n", wurds[3]);
            }
        }

        return true;
    }
    return false;
}

bool is_valid_algo_num(int algo_num)
{
    printf("VALID? Algo num:%d\n", algo_num);
    if (algo_num >= 0 && algo_num < mixr->algorithm_num)
    {
        printf("TRUE!\n");
        return true;
    }
    printf("FALSE!\n");
    return false;
}
