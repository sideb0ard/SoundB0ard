#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"
#include "stack.h"

typedef struct sequence_generator
{
    char pattern[1024];
    Stack *rpn_stack;
    void (*status)(void *self, wchar_t *wstring);
    int (*generate)(void *self);
}
sequence_generator;

sequence_generator *new_sequence_generator(char wurds[][SIZE_OF_WURD], int num_wurds);
void sequence_generator_change_pattern(sequence_generator *sg, char *pattern);
void sequence_generator_status(void *self, wchar_t *wstring);
int sequence_generator_generate(void *self);
