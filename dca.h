#pragma once

#include "defjams.h"
#include "modmatrix.h"
#include "synthfunctions.h"

#define AMP_MOD_RANGE -96 // -96dB

typedef struct dca {

    modmatrix *g_modmatrix;

    unsigned m_mod_source_eg;
    unsigned m_mod_source_amp_db;
    unsigned m_mod_source_velocity;
    unsigned m_mod_source_pan;

    global_dca_params *m_global_dca_params;

    double m_gain;

    unsigned int m_midi_velocity;

    double m_amplitude_db; // gui?
    double m_amplitude_control;

    double m_pan_control;

    double m_amp_mod_db;

    double m_eg_mod;

    double m_pan_mod;

} dca;

dca *new_dca(void);
void dca_initialize(dca *d);
void dca_set_midi_velocity(dca *self, unsigned int vel);
void dca_set_pan_control(dca *self, double pan);
void dca_reset(dca *self);
void dca_set_amplitude_db(dca *self, double amp);
void dca_set_amp_mod_db(dca *self, double mod);
void dca_set_eg_mod(dca *self, double mod);
void dca_set_pan_mod(dca *self, double mod);
void dca_update(dca *self);
void dca_gennext(dca *self, double left_input, double right_input,
                 double *left_output, double *right_output);
void dca_init_global_parameters(dca *self, global_dca_params *params);
