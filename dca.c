#include <math.h>
#include <stdlib.h>

#include "dca.h"
#include "defjams.h"
#include "utils.h"

DCA *new_dca()
{
    DCA *dca = calloc(1, sizeof(DCA));
    dca->m_amplitude_control = 1.0;
    dca->m_amp_mod_db = 0.0;
    dca->m_amplitude_db = 0.0;
    dca->m_eg_mod = 1.0;
    dca->m_pan_control = 0.0;
    dca->m_pan_mod = 0.0;
    dca->m_midi_velocity = 127;

    dca->global_modmatrix = NULL;
    dca->m_mod_source_eg = DEST_NONE;
    dca->m_mod_source_amp_db = DEST_NONE;
    dca->m_mod_source_velocity = DEST_NONE;
    dca->m_mod_source_pan = DEST_NONE;

    return dca;
}

void dca_set_midi_velocity(DCA *self, int vel) { self->m_midi_velocity = vel; }

void dca_set_pan_control(DCA *self, double pan) { self->m_pan_control = pan; }

void dca_reset(DCA *self)
{
    self->m_amp_mod_db = 0.0;
    self->m_eg_mod = 1.0;
}

void dca_set_amplitude_db(DCA *self, double amp)
{
    self->m_amplitude_db = amp;
    self->m_amplitude_control = pow((double)10.0, amp / (double)20.0);
}

void dca_set_amp_mod_db(DCA *self, double mod) { self->m_amp_mod_db = mod; }

void dca_set_eg_mod(DCA *self, double mod) { self->m_eg_mod = mod; }

void dca_set_pan_mod(DCA *self, double mod) { self->m_pan_mod = mod; }

void dca_update(DCA *self)
{
    if (self->m_eg_mod >= 0)
        self->m_gain = self->m_eg_mod;
    else
        self->m_gain = self->m_eg_mod + 1.0;

    self->m_gain *= pow(10.0, self->m_amp_mod_db / (double)20.0);
}

void dca_gennext(DCA *self, double left_input, double right_input,
                 double *left_output, double *right_output)
{
    double pan_total = self->m_pan_control + self->m_pan_mod;

    pan_total = fmin(pan_total, 1.0);
    pan_total = fmax(pan_total, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(pan_total, &pan_left, &pan_right);

    *left_output =
        pan_left * self->m_amplitude_control * left_input * self->m_gain;
    *right_output =
        pan_right * self->m_amplitude_control * right_input * self->m_gain;
}
