#pragma once

#include "defjams.h"
#include "pattern_generator.h"

enum
{
    GARAGE,
    HIPHOP,
    HATS,
    HATS_MASK,
    CLAPS,
    RAGGA_BEAT,
    RAGGA_SNARE,
    NUM_MARKOV_STYLES
};

typedef struct markov
{
    pattern_generator sg;
    unsigned int markov_type;
} markov;

pattern_generator *new_markov(unsigned int type);
void markov_generate(void *self, void *data);
void markov_status(void *self, wchar_t *status_string);
void markov_event_notify(void *self, unsigned int event_type);
void markov_set_debug(void *self, bool b);
void markov_set_type(markov *m, unsigned int type);
