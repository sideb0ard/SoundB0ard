#pragma once

#include <wchar.h>

enum sequence_gen_type
{
    EUCLIDEAN,
    BITSHIFT,
    MARKOV
};

typedef struct sequence_generator
{
    unsigned int type;
    void (*status)(void *self, wchar_t *wstring);
    int (*generate)(void *self, void *data);
    void (*event_notify)(void *self, unsigned int event_type);
} sequence_generator;

// void sequence_generator_status(void *self, wchar_t *wstring);
// int sequence_generator_generate(void *self);
