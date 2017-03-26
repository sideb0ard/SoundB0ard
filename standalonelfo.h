#pragma once

#include "oscillator.h"
#include "sound_generator.h"

typedef struct standalonelfo {
    SOUNDGEN sg;
    oscillator osc;
} standalonelfo;

standalonelfo *lfo_new_standalone(void);
double lfo_do_standalone(void *sg);

void lfo_status(void *self, wchar_t *ss);
double lfo_getvol(void *self);
void lfo_setvol(void *self, double v);
