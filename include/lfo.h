#pragma once

#include "oscillator.h"

typedef struct lfo
{
    oscillator osc;
} lfo;

lfo *lfo_new(void);

void lfo_set_sound_generator_interface(lfo *l);

double lfo_do_oscillate(oscillator *self, double *quad_phase_output);

void lfo_start_oscillator(oscillator *self);
void lfo_stop_oscillator(oscillator *self);
void lfo_reset_oscillator(oscillator *self);
