#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "sound_generator.h"

#define MAX_CMDS 10
#define MAX_CMD_LEN 64

typedef enum { TICK, S16TH, LOOP } frequency;

typedef struct algorithm {
    SOUNDGEN sound_generator;
    char command[MAX_CMD_LEN];
    char afterthought[5][MAX_CMD_LEN];
    unsigned int frequency;
    bool has_started;
    bool active;
} algorithm;

algorithm *new_algorithm(char *line);

int extract_cmds_from_line(algorithm *self, char *line);
void algorithm_replace_vars_in_cmd(char *updated_cmd, char *stored_cmd);
void algorithm_process_afterthought(algorithm *self);

double algorithm_gen_next(void *self);
void algorithm_status(void *self, wchar_t *ss);
void algorithm_setvol(void *self, double v);
double algorithm_getvol(void *self);
