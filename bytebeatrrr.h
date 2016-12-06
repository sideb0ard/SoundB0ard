#pragma once

#include "sound_generator.h"

typedef struct bytebeat
{
    SOUNDGEN sound_generator;
    char pattern[256];
} bytebeat;


bytebeat *new_bytebeat(char *pattern);

double bytes_gen_next(void *self);
void   bytes_status(void *self, char *ss);

