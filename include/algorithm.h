#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "sound_generator.h"

#define MAX_CMDS 10
#define MAX_CMD_LEN 4096

// typedef enum { TICK, S16TH, LOOP } frequency;

typedef struct algorithm
{
    soundgenerator sound_generator;
    char command[MAX_CMD_LEN];
    char afterthought[5][MAX_CMD_LEN];
    unsigned int frequency;
    int every_n; // i.e every_n frequency e.g. every 4 loops or every 3rd 16th
    int counter;
    bool has_started;
    bool active;
    bool debug;
    int target_sound_generator;
    int target_sound_generator_pattern_num;
    parceled_pattern original_pattern;
} algorithm;

algorithm *new_algorithm(int num_wurds, char wurds[][SIZE_OF_WURD]);
int extract_cmds_from_line(algorithm *a, int num_wurds,
                           char wurds[][SIZE_OF_WURD]);
void algorithm_replace_vars_in_cmd(char *updated_cmd, char *stored_cmd);
void algorithm_process_afterthought(algorithm *self);

stereo_val algorithm_gennext(void *self);
void algorithm_status(void *self, wchar_t *ss);
void algorithm_setvol(void *self, double v);
double algorithm_getvol(void *self);
void algorithm_start(void *self);
void algorithm_stop(void *self);
void algorithm_event_notify(void *self, unsigned int event_type);
