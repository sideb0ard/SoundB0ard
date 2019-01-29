#pragma once

#include "defjams.h"
#include "pattern_generator.h"

enum
{
    CLAP2,
    KICK2,
    CLAPS,
    GARAGE,
    HATS,
    HATS2,
    HATS_MASK,
    HIPHOP,
    HOUSE,
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
void markov_event_notify(void *self, broadcast_event event);
void markov_set_debug(void *self, bool b);
void markov_set_type(markov *m, unsigned int type);

void markov_pattern_generate(unsigned int markov_type, midi_event *midi_pattern);
