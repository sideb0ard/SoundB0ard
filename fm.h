#pragma once

#include <stdbool.h>

#include "envelope_generator.h"
#include "oscil.h"
#include "dca.h"
#include "sound_generator.h"

typedef struct FM FM;

typedef struct FM {
    // OSC **oscillators;
    // int num_oscil;
    SOUNDGEN sound_generator;

    envelope_generator *env;
    OSCIL *osc1;
    OSCIL *osc2;
    OSCIL *lfo;
    DCA *dca;
    bool note_on;
    float vol;

} FM;

FM *new_fm(double freq1, double freq2);
FM *new_fm_x(char *osc1_type, double osc1_fq, char *osc2_type, double osc2_fq);

double fm_gennext(void *self);
// void fm_gennext(void* self, double* frame_vals, int framesPerBuffer);
void fm_status(void *self, char *status_string);
double fm_getvol(void *self);
void fm_setvol(void *self, double v);
void mfm(void *self, double val);
void keypress_on(void *self);
void keypress_off(void *self);
