#pragma once

#include <stdbool.h>

#include "sound_generator.h"

#define MAX_CMDS 10
#define MAX_CMD_LEN 64

typedef enum { TICK, S16TH, LOOP } frequency;

typedef struct algorithm {
    SOUNDGEN sound_generator;
    char cmds[MAX_CMDS][MAX_CMD_LEN];
    int num_cmds;
    int cur_cmd;
    unsigned int frequency;
    bool has_started;
} algorithm;

algorithm *new_algorithm(char *line);

void extract_cmds_from_line(algorithm *self, char *line);
void algorithm_replace_vars_in_cmd(char *updated_cmd, char *stored_cmd);

double algorithm_gen_next(void *self);
void algorithm_status(void *self, char *ss);
void algorithm_setvol(void *self, double v);
