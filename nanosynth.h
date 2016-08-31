#pragma once

#include <stdbool.h>

#include "dca.h"
#include "envelope_generator.h"
#include "filter.h"
#include "keys.h"
#include "modmatrix.h"
#include "oscillator.h"
#include "sound_generator.h"

typedef struct nanosynth {
    SOUNDGEN sound_generator;

    melody_loop *mloops[10];
    int melody_loop_num;
    int melody_loop_cur;

    envelope_generator *eg1;

    oscillator *osc1;
    oscillator *osc2;
    oscillator *lfo;

    // FILTER_CSEM *filter;
    // FILTER_ONEPOLE *filter;
    filter *f;
    DCA *dca;

    int cur_octave;
    int sustain; // in ticks TODO: make better!

    bool note_on;
    bool m_filter_keytrack;

    float vol;

    modmatrix *m_modmatrix; // routing structure for sound generation
    // need these for mod matrix
    double m_default_mod_intensity; // 1.0
    double m_default_mod_range;     // 1.0
    double m_osc_fq_mod_range;
    double m_filter_mod_range;
    double m_eg1_dca_intensity;
    double m_eg1_osc_intensity;
    double m_filter_keytrack_intensity;

    // "gui" controls for oscillators
    unsigned m_osc_waveform;
    unsigned m_lfo_waveform;
    double m_lfo_amplitude;
    double m_lfo_rate;
    unsigned m_lfo_mode;

    // "gui" controls for Envelope Generator
    double m_attack_time_msec;
    double m_decay_time_msec;
    double m_sustain_level;
    double m_release_time_msec;
    bool m_reset_to_zero;
    unsigned m_legato_mode;

    // "gui" controls for DCA
    double m_pan_control;
    double m_volume_db;

} nanosynth;

nanosynth *new_nanosynth(void);

double nanosynth_gennext(void *self);
// void nanosynth_gennext(void* self, double* frame_vals, int framesPerBuffer);
void nanosynth_status(void *self, char *status_string);
double nanosynth_getvol(void *self);
void nanosynth_setvol(void *self, double v);
void note_on(nanosynth *self, int midi_num);
//void note_off(void *self, int midi_num);
void change_octave(void *self, int direction);
void nanosynth_change_osc_wave_form(nanosynth *self, int oscil);
void nanosynth_set_sustain(nanosynth *self, int sustain_val);
void nanosynth_add_melody_loop(void *self, melody_loop *mloop);
void nanosynth_update(nanosynth *self);
