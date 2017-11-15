#pragma once

#include "stack.h"

typedef struct bytebeat
{
    char pattern[1024];
    Stack *rpn_stack;
    unsigned int pattern_num;
} bytebeat;

void bytebeat_init(bytebeat *b);
double bytebeat_gennext(bytebeat *b);
void bytebeat_status(bytebeat *b, wchar_t *status_string);
