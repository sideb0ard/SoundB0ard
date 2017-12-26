#pragma once

#include <wchar.h>

typedef struct sequence_generator
{
    void (*status)(void *self, wchar_t *wstring);
    int (*generate)(void *self);
} sequence_generator;

void sequence_generator_status(void *self, wchar_t *wstring);
int sequence_generator_generate(void *self);
