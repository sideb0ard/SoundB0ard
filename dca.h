#pragma once

#include "defjams.h"
#include "modmatrix.h"

typedef struct dca {

    double m_gain;
    double m_amplitude_db; // gui?
    double m_amplitude_control;
    double m_pan_control;
    double m_amp_mod_db;
    double m_eg_mod;
    double m_pan_mod;
    int m_midi_velocity;

    modmatrix *global_modmatrix;

    unsigned m_mod_source_eg;
    unsigned m_mod_source_amp_db;
    unsigned m_mod_source_velocity;
    unsigned m_mod_source_pan;

} DCA;

DCA *new_dca(void);
void dca_set_midi_velocity(DCA *self, int vel);
void dca_set_pan_control(DCA *self, double pan);
void dca_reset(DCA *self);
void dca_set_amplitude_db(DCA *self, double amp);
void dca_set_amp_mod_db(DCA *self, double mod);
void dca_set_eg_mod(DCA *self, double mod);
void dca_set_pan_mod(DCA *self, double mod);
void dca_update(DCA *self);
void dca_gennext(DCA *self, double left_input, double right_input,
                 double *left_output, double *right_output);
