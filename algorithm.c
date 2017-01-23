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
    algorithm *a = calloc(1, sizeof(algorithm));

    printf("New algorithm!\n");
    if (extract_cmds_from_line(a, line)) {
        printf("Couldn't part commands from line\n");
        free(a);
        return NULL;
    }

    a->sound_generator.gennext = &algorithm_gen_next;
    a->sound_generator.status = &algorithm_status;
    a->sound_generator.setvol = &algorithm_setvol;
    a->sound_generator.type = ALGORITHM_TYPE;

    return a;
}

int extract_cmds_from_line(algorithm *self, char *line)
{
    printf("Extracting commands from %s\n", line);
    int num_cmds = 0;

    char *cmd, *last_s;
    char *sep = ";";
    for (cmd = strtok_r(line, sep, &last_s); cmd;
         cmd = strtok_r(NULL, sep, &last_s)) {

        if (strncmp("every loop", cmd, 11) == 0 && num_cmds == 0) {
            self->frequency = LOOP;
            num_cmds++;
        }
        else if (num_cmds == 1) {
            strncpy(self->command, cmd, MAX_CMD_LEN);
            printf("CMD %s\n", cmd);
            num_cmds++;
        }
        else if (num_cmds == 2) {
            int num_scanned =
                sscanf(cmd, "%s %s %s %s %s", self->afterthought[0],
                       self->afterthought[1], self->afterthought[2],
                       self->afterthought[3], self->afterthought[4]);
            printf("AFTERTHOUGHT %s (scanned %d)\n", cmd, num_scanned);
            if (num_scanned == 5 &&
                strncmp("%", self->afterthought[3], 2) == 0) {
                num_cmds++;
            }
        }
    }
    if (num_cmds != 3) {
        printf("DONK! Doesn't have 3 commands\n");
        return 1;
    }
    return 0;
}

void algorithm_replace_vars_in_cmd(char *updated_cmd, char *stored_cmd)
{
    char *toke, *last_toke;
    char *sep = " ";
    for (toke = strtok_r(stored_cmd, sep, &last_toke); toke;
         toke = strtok_r(NULL, sep, &last_toke)) {

        for (int i = 0; i < mixr->env_var_count; i++) {
            if (strncmp(mixr->environment[i].key, toke, ENVIRONMENT_KEY_SIZE) ==
                0) {
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
    if (get_environment_val(self->afterthought[0], &orig_val)) {
        printf("key not found\n");
        return;
    }

    if (strncmp(self->afterthought[1], "+=", 2) == 0) {
        update_environment(self->afterthought[0],
                           (orig_val += atoi(self->afterthought[2])) %
                               atoi(self->afterthought[4]));
    }
    if (strncmp(self->afterthought[1], "*=", 2) == 0) {
        update_environment(self->afterthought[0],
                           (orig_val *= atoi(self->afterthought[2])) %
                               atoi(self->afterthought[4]));
    }
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
            strncpy(stored_cmd, a->command, MAX_CMD_LEN);
            algorithm_replace_vars_in_cmd((char *)now_cmd, stored_cmd);
            interpret(now_cmd);
            algorithm_process_afterthought(a);
        }
        else if (mixr->tick % mixr->loop_len_in_ticks != 0) {
            a->has_started = false;
        }
        break;
    }
    return 0;
}

void algorithm_status(void *self, wchar_t *status_string)
{
    algorithm *a = (algorithm *)self;
    wcslcat(status_string, (wchar_t *)a->command, strlen(a->command));
    wcslcat(status_string, L" ", 2);
    wcslcat(status_string, (wchar_t *)a->afterthought[0],
            strlen(a->afterthought[0]));
    wcslcat(status_string, (wchar_t *)a->afterthought[1],
            strlen(a->afterthought[1]));
    wcslcat(status_string, (wchar_t *)a->afterthought[2],
            strlen(a->afterthought[2]));
}

void algorithm_setvol(void *self, double v)
{
    (void)self;
    (void)v;
    // no-op
}
