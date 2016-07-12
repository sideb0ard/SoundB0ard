#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "envelope_generator.h"

envelope_generator *new_envelope_generator()
{
    envelope_generator *eg =
        (envelope_generator *)calloc(1, sizeof(envelope_generator));
    if (eg == NULL) {
        printf("Oof\n");
        return NULL;
    }

    eg->m_state = OFFF;
    eg->m_eg_mode = ANALOG;
    eg->m_attack_time_msec = EG_DEFAULT_STATE;
    eg->m_decay_time_msec = EG_DEFAULT_STATE;
    eg->m_release_time_msec = EG_DEFAULT_STATE;
    eg->m_shutdown_time_msec = 100.0;
    eg->m_sustain_level = 0.707;
    eg->m_output_eg = true;

    set_eg_mode(eg, eg->m_eg_mode);

    eg->m_legato_mode = false;
    eg->m_reset_to_zero = false;

    return eg;
}

state get_state(envelope_generator *self) { return self->m_state; }

bool is_active(envelope_generator *self)
{
    if (self->m_state != RELEASE && self->m_state != 0)
        return true;
    return false;
}

bool can_note_off(envelope_generator *self)
{
    if (self->m_state != RELEASE && self->m_state != SHUTDOWN &&
        self->m_state != OFFF)
        return true;
    return false;
}

void reset(envelope_generator *self)
{
    self->m_state = OFFF;
    set_eg_mode(self, self->m_eg_mode);
    calculate_release_time(self);

    if (self->m_reset_to_zero) {
        self->m_envelope_output = 0.0;
    }
}

void set_eg_mode(envelope_generator *self, eg_mode mode)
{
    self->m_eg_mode = mode;
    if (self->m_eg_mode == ANALOG) {
        self->m_attack_tco = exp(-1.5); // fast attack
        self->m_decay_tco = exp(-4.95);
        self->m_release_tco = self->m_decay_tco;
    }
    else {
        self->m_attack_tco = 0.99999;
        self->m_decay_tco = exp(-11.05);
        self->m_release_tco = self->m_decay_tco;
    }

    calculate_attack_time(self);
    calculate_decay_time(self);
    calculate_release_time(self);
}

void calculate_attack_time(envelope_generator *self)
{
    double d_samples = SAMPLE_RATE * ((self->m_attack_time_msec) / 1000.0);
    self->m_attack_coeff =
        exp(-log((1.0 + self->m_attack_tco) / self->m_attack_tco) / d_samples);
    self->m_attack_offset =
        (1.0 + self->m_attack_tco) * (1.0 - self->m_attack_coeff);
}

void calculate_decay_time(envelope_generator *self)
{
    double d_samples = SAMPLE_RATE * ((self->m_decay_time_msec) / 1000.0);
    self->m_decay_coeff =
        exp(-log((1.0 + self->m_decay_tco) / self->m_decay_tco) / d_samples);
    self->m_decay_offset = (self->m_sustain_level - self->m_decay_tco) *
                           (1.0 - self->m_decay_coeff);
}

void calculate_release_time(envelope_generator *self)
{
    double d_samples = SAMPLE_RATE * ((self->m_release_time_msec) / 1000.0);
    self->m_release_coeff = exp(
        -log((1.0 + self->m_release_tco) / self->m_release_tco) / d_samples);
    self->m_release_offset =
        self->m_release_tco * (1.0 - self->m_release_coeff);
}

void set_attack_time_msec(envelope_generator *self, double time)
{
    self->m_attack_time_msec = time;
    calculate_attack_time(self);
}

void set_decay_time_msec(envelope_generator *self, double time)
{
    self->m_decay_time_msec = time;
    calculate_decay_time(self);
}

void set_release_time_msec(envelope_generator *self, double time)
{
    self->m_release_time_msec = time;
    calculate_release_time(self);
}

void set_sustain_level(envelope_generator *self, double level)
{
    self->m_sustain_level = level;
    calculate_decay_time(self);
    if (self->m_state != RELEASE)
        calculate_release_time(self);
}

void start_eg(envelope_generator *self)
{
    if (self->m_legato_mode && self->m_state != OFFF &&
        self->m_state != RELEASE)
        return;
    // reset(self);
    self->m_state = ATTACK;
}

void eg_release(envelope_generator *self)
{
    if (self->m_state == SUSTAIN)
        self->m_state = RELEASE;
}

void stop_eg(envelope_generator *self) { self->m_state = OFFF; }

double env_generate(envelope_generator *self, double *p_biased_output)
{
    switch (self->m_state) {
    case OFFF: {
        if (self->m_reset_to_zero)
            self->m_envelope_output = 0.0;
        break;
    }
    case ATTACK: {
        self->m_envelope_output =
            self->m_attack_offset +
            self->m_envelope_output * self->m_attack_coeff;
        if (self->m_envelope_output >= 1.0 || self->m_attack_time_msec <= 0.0) {
            self->m_envelope_output = 1.0;
            self->m_state = DECAY;
        }
        break;
    }
    case DECAY: {
        self->m_envelope_output = self->m_decay_offset +
                                  self->m_envelope_output * self->m_decay_coeff;
        if (self->m_envelope_output <= self->m_sustain_level ||
            self->m_decay_time_msec <= 1.0) {
            self->m_envelope_output = self->m_sustain_level;
            self->m_state = SUSTAIN;
        }
        break;
    }
    case SUSTAIN: {
        self->m_envelope_output = self->m_sustain_level;
        break;
    }
    case RELEASE: {
        self->m_envelope_output =
            self->m_release_offset +
            self->m_envelope_output * self->m_release_coeff;
        if (self->m_envelope_output <= 0.0 ||
            self->m_release_time_msec <= 0.0) {
            self->m_envelope_output = 0.0;
            self->m_state = OFFF;
        }
        break;
    }
    case SHUTDOWN: {
        if (self->m_reset_to_zero) {
            self->m_envelope_output += self->m_inc_shutdown;
            if (self->m_envelope_output <= 0) {
                self->m_state = OFFF;
                self->m_envelope_output = 0.0;
                break;
            }
        }
        else {
            self->m_state = OFFF;
        }
        break;
    }
    }
    if (p_biased_output != NULL)
        *p_biased_output = self->m_envelope_output - self->m_sustain_level;

    return self->m_envelope_output;
}

void note_off(envelope_generator *self)
{
    if (self->m_envelope_output > 0)
        self->m_state = RELEASE;
    else
        self->m_state = OFFF;
}
