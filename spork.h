#pragma once

#include "oscillator.h"
#include "sound_generator.h"

typedef struct spork {
    SOUNDGEN sg;
    oscillator osc;
} spork;

spork *new_spork(void);
double spork_gennext(void *sg);

void spork_status(void *self, wchar_t *ss);
double spork_getvol(void *self);
void spork_setvol(void *self, double v);
