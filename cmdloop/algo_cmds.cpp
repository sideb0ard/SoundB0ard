#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algo_cmds.h>
#include <mixer.h>

extern mixer *mixr;

bool parse_algo_cmd(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    //    if (strncmp("algo", wurds[0], 4) == 0)
    //    {
    //        int algo_num = atoi(wurds[1]);
    //        if (mixer_is_valid_algo(mixr, algo_num))
    //        {
    //            algorithm *a = mixr->algorithms[algo_num];
    //
    //            if (strncmp("append", wurds[2], 6) == 0)
    //            {
    //                algorithm_append_target(a, wurds[3]);
    //            }
    //            else if (strncmp("on", wurds[2], 2) == 0 ||
    //                     strncmp("start", wurds[2], 4) == 0)
    //                a->active = true;
    //            else if (strncmp("off", wurds[2], 3) == 0 ||
    //                     strncmp("stop", wurds[2], 4) == 0)
    //                a->active = false;
    //            else if (strncmp("step", wurds[2], 4) == 0)
    //            {
    //                int step = atoi(wurds[3]);
    //                a->process_step = step;
    //            }
    //            else if (strncmp("event", wurds[2], 5) == 0)
    //            {
    //                int err = algorithm_set_event_type_from_string(a,
    //                wurds[3]); if (err != 0)
    //                    printf("Nah!\n");
    //            }
    //            else if (strncmp("debug", wurds[2], 5) == 0)
    //            {
    //                bool debug = atoi(wurds[3]);
    //                algorithm_set_debug(a, debug);
    //            }
    //            else if (strncmp("process", wurds[2], 10) == 0)
    //            {
    //                if (strncmp(wurds[3], "over", 4) == 0)
    //                    algorithm_set_var_select_type(a, VAR_OSC);
    //                else
    //                    algorithm_set_var_select_type(a, VAR_STEP);
    //            }
    //            else if (strncmp("var_select", wurds[2], 10) == 0)
    //            {
    //                int var_select_type =
    //                    algorithm_get_var_select_type_from_string(wurds[3]);
    //                algorithm_set_var_select_type(a, var_select_type);
    //            }
    //            else if (strncmp("var_list", wurds[2], 8) == 0 ||
    //                     strncmp("cmd", wurds[2], 3) == 0)
    //            {
    //                int wurds_left = num_wurds - 3;
    //                int size_of_wurds_left =
    //                    wurds_left; // pre-alloc spaces and NULL
    //                for (int i = 3; i < num_wurds; ++i)
    //                {
    //                    size_of_wurds_left += strlen(wurds[i]);
    //                }
    //                char concat_wurds[size_of_wurds_left];
    //                memset(concat_wurds, 0, size_of_wurds_left);
    //                for (int i = 3; i < num_wurds; ++i)
    //                {
    //                    strcat(concat_wurds, wurds[i]);
    //                    if (i < num_wurds - 1)
    //                        strcat(concat_wurds, " ");
    //                }
    //
    //                if (strncmp("var_list", wurds[2], 8) == 0)
    //                    algorithm_set_var_list(a, concat_wurds);
    //                else
    //                    algorithm_set_cmd(a, concat_wurds);
    //            }
    //        }
    //
    //        return true;
    //    }
    return false;
}
