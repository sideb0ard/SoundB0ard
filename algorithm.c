#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algorithm.h"
#include "cmdloop.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

algorithm *new_algorithm(int num_wurds, char wurds[][SIZE_OF_WURD])
{
    algorithm *a = (algorithm *)calloc(1, sizeof(algorithm));

    printf("New algorithm!\n");
    if (extract_cmds_from_line(a, num_wurds, wurds))
    {
        printf("Couldn't part commands from line\n");
        free(a);
        return NULL;
    }

    a->sound_generator.gennext = &algorithm_gennext;
    a->sound_generator.ps_status = &algorithm_status;
    a->sound_generator.full_status = &algorithm_status;
    a->sound_generator.setvol = &algorithm_setvol;
    a->sound_generator.getvol = &algorithm_getvol;
    a->sound_generator.start = &algorithm_start;
    a->sound_generator.stop = &algorithm_stop;
    a->sound_generator.event_notify = &algorithm_event_notify;
    a->sound_generator.type = ALGORITHM_TYPE;

    a->counter = 0;
    a->has_started = false;

    return a;
}

static bool should_take_action(algorithm *a)
{
    bool b = false;
    if (a->counter % a->every_n == 0)
        b = true;
    a->counter++;
    return b;
}

void algorithm_event_notify(void *self, unsigned int event_type)
{
    algorithm *a = (algorithm *)self;

    switch (event_type)
    {
    case (TIME_THIRTYSECOND_TICK):
        if (a->frequency == TIME_THIRTYSECOND_TICK)
        {
            if (should_take_action(a))
            {
                printf("THIR32!\n");
            }
        }
        break;
    case (TIME_SIXTEENTH_TICK):
        if (a->frequency == TIME_SIXTEENTH_TICK)
        {
            if (should_take_action(a))
            {
                printf("T16thth!\n");
            }
        }
        break;
    case (TIME_EIGHTH_TICK):
        if (a->frequency == TIME_EIGHTH_TICK)
        {
            if (should_take_action(a))
            {
                printf("beep8!\n");
            }
            a->counter++;
        }
        break;
    case (TIME_QUARTER_TICK):
        if (a->frequency == TIME_QUARTER_TICK)
        {
            if (should_take_action(a))
            {
                printf("beep4!\n");
            }
        }
        break;
    case (TIME_START_OF_LOOP_TICK):
        if (a->frequency == TIME_START_OF_LOOP_TICK)
        {
            if (should_take_action(a))
            {
                printf("beepLoop!\n");
            }
        }
        break;
    }
}

int extract_cmds_from_line(algorithm *a, int num_wurds,
                           char wurds[][SIZE_OF_WURD])
{

    if (strncmp(wurds[0], "every", 5) == 0)
    {
        int every_n = atoi(wurds[1]);
        printf("Every %d!\n", every_n);
        if (every_n == 0)
        {
            printf("don't be daft, cannae dae 0 times.\n");
            return -1;
        }
        a->every_n = every_n;

        if (strncmp(wurds[2], "loop", 4) == 0)
        {
            a->frequency = TIME_START_OF_LOOP_TICK;
            printf("Loop!\n");
        }
        else if (strncmp(wurds[2], "4th", 4) == 0)
        {
            a->frequency = TIME_QUARTER_TICK;
            printf("Quart!\n");
        }
        else if (strncmp(wurds[2], "8th", 4) == 0)
        {
            a->frequency = TIME_EIGHTH_TICK;
            printf("Eighth!\n");
        }
        else if (strncmp(wurds[2], "16th", 4) == 0)
        {
            a->frequency = TIME_SIXTEENTH_TICK;
            printf("Sizteenth!\n");
        }
        else if (strncmp(wurds[2], "32nd", 4) == 0)
        {
            a->frequency = TIME_THIRTYSECOND_TICK;
            printf("ThirzztySecdon!\n");
        }
    }

    return 0;
}

void algorithm_replace_vars_in_cmd(char *updated_cmd, char *stored_cmd)
{
    char *toke, *last_toke;
    char const *sep = " ";
    for (toke = strtok_r(stored_cmd, sep, &last_toke); toke;
         toke = strtok_r(NULL, sep, &last_toke))
    {

        for (int i = 0; i < mixr->env_var_count; i++)
        {
            if (strncmp(mixr->environment[i].key, toke, ENVIRONMENT_KEY_SIZE) ==
                0)
            {
                itoa(mixr->environment[i].val, toke);
            }
        }
        strcat(updated_cmd, toke);
        strcat(updated_cmd, " ");
    }
}

void algorithm_process_afterthought(algorithm *self)
{
    int orig_val = 0;
    if (get_environment_val(self->afterthought[0], &orig_val))
    {
        printf("key not found\n");
        return;
    }

    if (strncmp(self->afterthought[1], "+=", 2) == 0)
    {
        update_environment(self->afterthought[0],
                           (orig_val += atoi(self->afterthought[2])) %
                               atoi(self->afterthought[4]));
    }
    if (strncmp(self->afterthought[1], "*=", 2) == 0)
    {
        update_environment(self->afterthought[0],
                           (orig_val *= atoi(self->afterthought[2])) %
                               atoi(self->afterthought[4]));
    }
}

stereo_val algorithm_gennext(void *self)
{
    algorithm *a = (algorithm *)self;
    // switch (a->frequency)
    //{
    // case LOOP:
    //    if (!a->has_started && (mixr->timing_info.midi_tick %
    //                                mixr->timing_info.loop_len_in_ticks ==
    //                            0))
    //    {
    //        a->has_started = true;
    //        char now_cmd[MAX_CMD_LEN] = {0};
    //        char stored_cmd[MAX_CMD_LEN] = {0};
    //        strncpy(stored_cmd, a->command, MAX_CMD_LEN);
    //        algorithm_replace_vars_in_cmd((char *)now_cmd, stored_cmd);
    //        interpret(now_cmd);
    //        algorithm_process_afterthought(a);
    //    }
    //    else if (mixr->timing_info.midi_tick %
    //                 mixr->timing_info.loop_len_in_ticks !=
    //             0)
    //    {
    //        a->has_started = false;
    //    }
    //    break;
    //}
    return (stereo_val){0, 0};
}

void algorithm_status(void *self, wchar_t *status_string)
{
    algorithm *a = (algorithm *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             WANSI_COLOR_RED "[ALGO]%s Post: %s %s %s %s %s", a->command,
             a->afterthought[0], a->afterthought[1], a->afterthought[2],
             a->afterthought[3], a->afterthought[4]);
    wcscat(status_string, WANSI_COLOR_RESET);
}

void algorithm_setvol(void *self, double v)
{
    (void)self;
    (void)v;
    // no-op
}

double algorithm_getvol(void *self)
{
    (void)self;
    return 0.0;
}

void algorithm_start(void *self) { (void)self; }
void algorithm_stop(void *self) { (void)self; }
