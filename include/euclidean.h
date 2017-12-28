#pragma once

#include "defjams.h"
#include "sequence_generator.h"

typedef struct euclidean
{
    sequence_generator sg;
    int num_hits;
    int num_steps;
} euclidean;

sequence_generator *new_euclidean(int num_hits, int num_steps);
int euclidean_generate(void *self, void *data);
void euclidean_status(void *self, wchar_t *status_string);
int create_euclidean_rhythm(int num_hits, int num_steps);
