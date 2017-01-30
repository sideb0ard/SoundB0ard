#pragma once

#include "defjams.h"
#include "modmatrix.h"

#define AMP_MOD_RANGE -96 // -96dB

typedef struct dca {

    double m_gain;
    double m_amplitude_db; // gui?
    double m_amplitude_control;
    double m_pan_control;
    double m_amp_mod_db;
    double m_eg_mod;
    double m_pan_mod;
    int m_midi_velocity;

    modmatrix *g_modmatrix;

    unsigned m_mod_source_eg;
    unsigned m_mod_source_amp_db;
    unsigned m_mod_source_velocity;
    unsigned m_mod_source_pan;

} dca;

dca *new_dca(void);
void dca_set_midi_velocity(dca *self, int vel);
void dca_set_pan_control(dca *self, double pan);
void dca_reset(dca *self);
void dca_set_amplitude_db(dca *self, double amp);
void dca_set_amp_mod_db(dca *self, double mod);
void dca_set_eg_mod(dca *self, double mod);
void dca_set_pan_mod(dca *self, double mod);
void dca_update(dca *self);
void dca_gennext(dca *self, double left_input, double right_input,
                 double *left_output, double *right_output);
