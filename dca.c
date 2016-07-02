#include <math.h>
#include <stdlib.h>

#include "dca.h"
#include "defjams.h"

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

    return dca;
}

void set_midi_velocity(DCA *self, int vel) { self->m_midi_velocity = vel; }

void set_pan_control(DCA *self, double pan) { self->m_pan_control = pan; }

void dca_reset(DCA *self)
{
    self->m_amp_mod_db = 0.0;
    self->m_eg_mod = 1.0;
}

void set_amplitude_db(DCA *self, double amp)
{
    self->m_amplitude_db = amp;
    self->m_amplitude_control = pow((double)10.0, amp / (double)20.0);
}

void set_amp_mod_db(DCA *self, double mod) { self->m_amp_mod_db = mod; }

void set_eg_mod(DCA *self, double mod) { self->m_eg_mod = mod; }

void set_pan_mod(DCA *self, double mod) { self->m_pan_mod = mod; }

void update(DCA *self)
{
    if (self->m_eg_mod >= 0)
        self->m_gain = self->m_eg_mod;
    else
        self->m_gain = self->m_eg_mod + 1.0;

    self->m_gain *= pow(10.0, self->m_amp_mod_db / (double)20.0);
}

void gennext(DCA *self)
{
    // TODO
    // double pan_total = self->m_pan_control + self->pan_mod;
    // pan_total = fmin(pan_total, 1.0);
    // pan_total = fmax(pan_total, 1.0);
    // double pan_left = 0.707;
    // double pan_right = 0.707;
    // calcula
}
