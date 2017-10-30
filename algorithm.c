#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algorithm.h"
#include "cmdloop.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

algorithm *new_algorithm(char *line)
{
    algorithm *a = (algorithm *)calloc(1, sizeof(algorithm));

    printf("New algorithm!\n");
    if (extract_cmds_from_line(a, line))
    {
        printf("Couldn't part commands from line\n");
        free(a);
        return NULL;
    }

    a->sound_generator.gennext = &algorithm_gennext;
    a->sound_generator.status = &algorithm_status;
    a->sound_generator.setvol = &algorithm_setvol;
    a->sound_generator.getvol = &algorithm_getvol;
    a->sound_generator.start = &algorithm_start;
    a->sound_generator.stop = &algorithm_stop;
    a->sound_generator.event_notify = &algorithm_event_notify;
    a->sound_generator.type = ALGORITHM_TYPE;

    return a;
}

void algorithm_event_notify(void *self, unsigned int event_type)
{
    (void)self;
    (void)event_type;
}

int extract_cmds_from_line(algorithm *self, char *line)
{
    printf("Extracting commands from %s\n", line);
    int num_cmds = 0;

    char *cmd, *last_s;
    char const *sep = ";";
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s))
    {

        printf("Cmd: %s\n", cmd);

        if (strncmp("every loop", cmd, 10) == 0 && num_cmds == 0)
        {
            self->frequency = LOOP;
            num_cmds++;
            printf("Every LOOP numcmds: %d\n", num_cmds);
        }
        else if (num_cmds == 1)
        {
            strncpy(self->command, cmd, MAX_CMD_LEN);
            printf("CMD %s\n", cmd);
            num_cmds++;
        }
        else if (num_cmds == 2)
        {
            int num_scanned =
                sscanf(cmd, "%s %s %s %s %s", self->afterthought[0],
                       self->afterthought[1], self->afterthought[2],
                       self->afterthought[3], self->afterthought[4]);
            printf("AFTERTHOUGHT %s (scanned %d)\n", cmd, num_scanned);
            if (num_scanned == 5 && strncmp("%", self->afterthought[3], 2) == 0)
            {
                num_cmds++;
            }
        }
    }
    if (num_cmds != 3)
    {
        printf("DONK! Doesn't have 3 commands\n");
        return 1;
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
    switch (a->frequency)
    {
    case LOOP:
        if (!a->has_started && (mixr->timing_info.midi_tick %
                                    mixr->timing_info.loop_len_in_ticks ==
                                0))
        {
            a->has_started = true;
            char now_cmd[MAX_CMD_LEN] = {0};
            char stored_cmd[MAX_CMD_LEN] = {0};
            strncpy(stored_cmd, a->command, MAX_CMD_LEN);
            algorithm_replace_vars_in_cmd((char *)now_cmd, stored_cmd);
            interpret(now_cmd);
            algorithm_process_afterthought(a);
        }
        else if (mixr->timing_info.midi_tick %
                     mixr->timing_info.loop_len_in_ticks !=
                 0)
        {
            a->has_started = false;
        }
        break;
    }
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
