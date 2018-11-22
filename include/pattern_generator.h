#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

enum pattern_gen_type
{
    EUCLIDEAN,
    BITSHIFT,
    MARKOV,
    NUM_GEN_TYPES,
};

typedef struct pattern_generator
{
    unsigned int type;
    bool debug;
    void (*status)(void *self, wchar_t *wstring);
    void (*generate)(void *self, void *data);
    void (*set_debug)(void *self, bool b);
    void (*event_notify)(void *self, unsigned int event_type);
} pattern_generator;
