#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"

#define MAX_CMDS 10
#define MAX_CMD_LEN 4096

#define MAX_VAR_KEY_LEN 128
#define MAX_VAR_VAL_LEN 128

#define MAX_LIST_ITEMS 64

enum
{
    INC_OP,
    SUB_OP,
    MULTI_OP,
    DIV_OP,
    RAND_OP,
};

enum
{
    VAR_RAND,
    VAR_OSC,
    VAR_STEP,
    MAX_VAR_SELECT_TYPES,
};

typedef struct algo_environment
{
    int variable_list_idx;
    int variable_list_len;
    char variable_list_string[MAX_STATIC_STRING_SZ];
    char variable_list_vals[MAX_LIST_ITEMS][MAX_VAR_VAL_LEN];

    unsigned int target_soundgen;
    int target_action_counter;

} algo_environment;

// process_type
enum
{
    EVERY,
    OVER,
    MAX_ALGO_PROCESS_TYPE,
};

typedef struct algorithm
{
    // e.g. every 3 bar (step="x y z") loop %d scramble
    unsigned int event_type;   // e.g. MIDI_TICK, SIXTEENTH, BAR
    unsigned int process_type; // EVERY, OVER * OVER only makes sense with OSC
    unsigned int process_step;
    unsigned int process_step_counter;
    unsigned int var_select_type; // RAND, OSC or STEP

    bool has_env;
    algo_environment env;

    char command[MAX_CMD_LEN];
    char runnable_command[MAX_CMD_LEN];

    char input_line[MAX_CMD_LEN];

    bool active;
    bool debug;

} algorithm;

algorithm *new_algorithm(int num_wurds, char wurds[][SIZE_OF_WURD]);
bool extract_cmds_from_line(algorithm *a, int num_wurds,
                            char wurds[][SIZE_OF_WURD]);
void algorithm_replace_vars_in_cmd(algorithm *a);

void algorithm_status(void *self, wchar_t *ss);
void algorithm_start(algorithm *a);
void algorithm_stop(algorithm *a);
void algorithm_event_notify(void *self, unsigned int event_type);
int algorithm_get_event_type_from_string(char *wurd);
int algorithm_get_var_select_type_from_string(char *wurd);
void algorithm_set_var_select_type(algorithm *s, unsigned int var_select_type);
