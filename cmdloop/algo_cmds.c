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

            return true;
        }
    }
    return false;
}

bool is_valid_algo_num(int algo_num)
{
    printf("VALID? Algo num:%d\n", algo_num);
    if (mixr->algorithms[algo_num] && algo_num >= 0 &&
        algo_num < mixr->algorithm_num)
    {
        printf("TRUE!\n");
        return true;
    }
    return false;
}
