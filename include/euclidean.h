#pragma once

#include "defjams.h"
#include "sequence_generator.h"

enum euclidean_mode {
    EUCLID_STATIC,
    EUCLID_UP,
    EUCLID_DOWN,
    EUCLID_RANDOM,
    EUCLID_ROTATE,
    EUCLID_NUM_MODES
};

typedef struct euclidean
{
    sequence_generator sg;
    int num_hits;
    int num_steps;
    unsigned int mode;
    int actual_num_hits;
    int actual_num_steps;
} euclidean;

sequence_generator *new_euclidean(int num_hits, int num_steps);
int euclidean_generate(void *self, void *data);
void euclidean_status(void *self, wchar_t *status_string);
void euclidean_event_notify(void *self, unsigned int event_type);

int create_euclidean_rhythm(int num_hits, int num_steps);
void euclidean_change_hits(euclidean *e, int num_hits);
void euclidean_change_steps(euclidean *e, int num_steps);
void euclidean_change_mode(euclidean *e, unsigned int mode);
