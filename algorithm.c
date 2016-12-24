#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algorithm.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

algorithm *new_algorithm(char *line)
{
    algorithm *a = calloc(1, sizeof(algorithm));

    printf("New algorithm!\n");
    extract_cmds_from_line(a, line);

    a->sound_generator.gennext = &algorithm_gen_next;
    a->sound_generator.status = &algorithm_status;
    a->sound_generator.setvol = &algorithm_setvol;

    return a;
}

void extract_cmds_from_line(algorithm *self, char *line)
{
    printf("Extracting commands from %s\n", line);
    char *cmd, *last_s;
    char *sep = ";";
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s)) {

        if (strncmp("every loop", cmd, 11) == 0) {
            self->frequency = LOOP;
        }
        else {
            strncpy(self->cmds[self->num_cmds++], cmd, MAX_CMD_LEN);
            printf("CMD %s\n", cmd);
        }
    }
}

void algorithm_replace_vars_in_cmd(char *updated_cmd, char *stored_cmd)
{
    printf("ALGO REPLACED CALLED - ORIGF CMD: %s\n", stored_cmd);
    char *toke, *last_toke;
    char *sep = " ";
    for (toke = strtok_r(stored_cmd, sep, &last_toke); toke;
         toke = strtok_r(NULL, sep, &last_toke)) {

        for (int i = 0; i < mixr->env_var_count; i++) {
            if (strncmp(mixr->environment[i].key, toke, ENVIRONMENT_KEY_SIZE) ==
                0) {
                printf("Replacing %s with %d\n", toke,
                       mixr->environment[i].val);
                itoa(mixr->environment[i].val, toke);
            }
        }
        strcat(updated_cmd, toke);
        strcat(updated_cmd, " ");
    }
    printf("UPDATED CMD: %s\n", updated_cmd);
}

double algorithm_gen_next(void *self)
{
    algorithm *a = (algorithm *)self;
    switch (a->frequency) {
    case LOOP:
        if (!a->has_started && (mixr->tick % mixr->loop_len_in_ticks == 0)) {
            a->has_started = true;
            char now_cmd[MAX_CMD_LEN] = {0};
            char stored_cmd[MAX_CMD_LEN] = {0};
            for (int i = 0; i < a->num_cmds; i++) {
                strncpy(stored_cmd, a->cmds[i], MAX_CMD_LEN);
                algorithm_replace_vars_in_cmd((char *)now_cmd, stored_cmd);
            }
        }
        else if (mixr->tick % mixr->loop_len_in_ticks != 0) {
            a->has_started = false;
        }
        break;
    }
    return 0;
}

void algorithm_status(void *self, char *status_string)
{
    algorithm *a = (algorithm *)self;
    // TODO - check for string length here
    for (int i = 0; i < a->num_cmds; i++) {
        strncat(status_string, a->cmds[i], strlen(a->cmds[i]));
        strncat(status_string, " ", 2);
    }
}

void algorithm_setvol(void *self, double v)
{
    // no-op
}
