#pragma once

#include "bytebeat/stack.h"
#include "sound_generator.h"
#include <wchar.h>

typedef struct bytebeat {
    SOUNDGEN sound_generator;
    char pattern[256];
    Stack *rpn_stack;
    double vol;
} bytebeat;

bytebeat *new_bytebeat(char *pattern);

double bytes_gen_next(void *self);
void bytes_status(void *self, wchar_t *ss);
void bytes_setvol(void *self, double v);
