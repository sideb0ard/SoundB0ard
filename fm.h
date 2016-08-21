#pragma once

#include <stdbool.h>

#include "dca.h"
#include "envelope_generator.h"
#include "filter.h"
#include "filter_ckthreefive.h"
#include "filter_onepole.h"
#include "keys.h"
#include "oscil.h"
#include "sound_generator.h"

typedef struct FM FM;

typedef struct FM {
    // OSC **oscillators;
    // int num_oscil;
    SOUNDGEN sound_generator;

    melody_loop *mloops[10];
    int melody_loop_num;
    int melody_loop_cur;

    envelope_generator *env;
    OSCIL *osc1;
    OSCIL *osc2;
    OSCIL *lfo;
    // FILTER_CSEM *filter;
    // FILTER_ONEPOLE *filter;
    FILTER_CK35 *filter;
    DCA *dca;
    int cur_octave;
    int sustain; // in ticks TODO: make better!
    bool note_on;
    bool m_filter_keytrack;
    double m_filter_keytrack_intensity;
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
void keypress_on(FM *self, double freq);
void keypress_off(void *self);
void change_octave(void *self, int direction);
void fm_change_osc_wave_form(FM *self, int oscil);
void fm_set_sustain(FM *self, int sustain_val);
void fm_add_melody_loop(void *self, melody_loop *mloop);
