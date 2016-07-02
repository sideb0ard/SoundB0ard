#pragma once

#include "defjams.h"

typedef struct dca {

    double m_gain;
    double m_amplitude_db; // gui?
    double m_amplitude_control;
    double m_pan_control;
    double m_amp_mod_db;
    double m_eg_mod;
    double m_pan_mod;
    int m_midi_velocity;

} DCA;

DCA *new_dca(void);
void set_midi_velocity(DCA *self, int vel);
void set_pan_control(DCA *self, double pan);
void dca_reset(DCA *self);
void set_amplitude_db(DCA *self, double amp);
void set_amp_mod_db(DCA *self, double mod);
void set_eg_mod(DCA *self, double mod);
void set_pan_mod(DCA *self, double mod);
void update(DCA *self);
void gennext(DCA *self);
