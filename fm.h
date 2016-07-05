#pragma once

#include <stdbool.h>

#include "dca.h"
#include "envelope_generator.h"
#include "filter.h"
#include "filter_onepole.h"
#include "oscil.h"
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
    FILTER_ONEPOLE *filter;
    DCA *dca;
    int cur_octave;
    int wave_form; // defined in defjams
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
void change_octave(void *self, int direction);
void fm_change_osc_wave_form(void *self);
