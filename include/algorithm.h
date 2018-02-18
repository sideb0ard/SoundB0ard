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
    LIST_TYPE,
    SCALAR_TYPE,
};

enum
{
    INC_OP,
    SUB_OP,
    MULTI_OP,
    DIV_OP,
    RAND_OP,
};

typedef struct algo_environment
{
    unsigned int type; // list or step
    char variable_key[MAX_VAR_KEY_LEN];

    // only used for list type
    int list_idx;
    int list_len;
    char variable_list_vals[MAX_LIST_ITEMS][MAX_VAR_VAL_LEN];

    // only used for step type
    unsigned int op;
    char variable_scalar_value[MAX_VAR_VAL_LEN];
} algo_environment;

typedef struct algorithm
{
    int every_n; // i.e every_n frequency e.g. every 4 loops or every 3rd 16th
    unsigned int frequency;
    int counter; // keeps track of which n we're on

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
