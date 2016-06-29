#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "envelope_generator.h"
#include "defjams.h"

envelope_generator *new_envelope_generator()
{
    envelope_generator *eg = (envelope_generator*)
                              calloc(1, sizeof(envelope_generator));
    if ( eg == NULL ) {
        printf("Oof\n");
        return NULL;
    }

    eg->m_state = OFF;
    eg->m_eg_mode = ANALOG;

    return eg;
}

state get_state(envelope_generator *self)
{
    return self->m_state;
}

bool is_active(envelope_generator *self)
{
    if (self->m_state != RELEASE && self->m_u_state != 0)
        return true;
    return false;
}

bool can_note_off(envelope_generator *self)
{
    if (self->m_u_state != RELEASE
            && self->m_u_state != SHUTDOWN
            && self->m_u_state != OFF)
        return true;
    return false;
}

void reset(envelope_generator *self)
{
    self->m_u_state = OFF;
    // set_eg_mode(self, self->m_u_eg_mode);  // whuh?
    calculate_release_time(self);

    if (self->m_b_reset_to_zero) {
        self->m_d_envelope_output = 0.0;
    }
}

void set_eg_mode(envelope_generator *self, eg_mode mode)
{
    self->m_u_eg_mode = mode;
    if (self->m_u_eg_mode == ANALOG) {
        self->m_d_attack_tco  = exp(-1.5); // fast attack
        self->m_d_decay_tco   = exp(-4.95);
        self->m_d_release_tco = self->m_d_decay_tco;
    } else {
        self->m_d_attack_tco  = 0.99999;
        self->m_d_decay_tco   = exp(-11.05);
        self->m_d_release_tco = self->m_d_decay_tco;
    }

    calculate_attack_time(self);
    calculate_decay_time(self);
    calculate_release_time(self);
}

void calculate_attack_time(envelope_generator *self)
{
    double d_samples = self->m_d_samplerate*((self->m_d_attack_time_msec)/1000.0);
    self->m_d_attack_coeff = exp(-log((1.0 + self->m_d_attack_tco)/self->m_d_attack_tco)
                             / d_samples);
    self->m_d_attack_offset = (1.0 + self->m_d_attack_tco)*(1.0 - self->m_d_attack_coeff);
}

void calculate_decay_time(envelope_generator *self)
{
    double d_samples = self->m_d_samplerate*((self->m_d_decay_time_msec)/1000.0);
    self->m_d_decay_coeff = exp(-log((1.0 + self->m_d_decay_tco)/self->m_d_decay_tco)
                             / d_samples);
    self->m_d_decay_offset = (self->m_d_sustain_level - self->m_d_decay_tco)
                             * (1.0 - self->m_d_decay_coeff);
}

void calculate_release_time(envelope_generator *self)
{
    double d_samples = self->m_d_samplerate*((self->m_d_release_time_msec)/1000.0);
    self->m_d_release_coeff = exp(-log((1.0 + self->m_d_release_tco)/self->m_d_release_tco)
                             / d_samples);
    self->m_d_release_offset = self->m_d_release_tco * (1.0 - self->m_d_release_coeff);
                             
}

void set_attack_time_msec(envelope_generator *self, double time)
{
    self->m_d_attack_time_msec = time;
    calculate_attack_time(self);
}

void set_decay_time_msec(envelope_generator *self, double time)
{
    self->m_d_decay_time_msec = time;
    calculate_decay_time(self);
}

void set_release_time_msec(envelope_generator *self, double time)
{
    self->m_d_release_time_msec = time;
    calculate_release_time(self);
}

void set_sustain_level(envelope_generator *self, double level)
{
    self->m_d_sustain_level = level;
    calculate_decay_time(self);
    if (self->m_u_state != RELEASE)
        calculate_release_time(self);
}

void set_sample_rate(envelope_generator *self, double samplerate)
{
    self->m_d_samplerate = samplerate;
    calculate_attack_time(self);
    calculate_decay_time(self);
    calculate_release_time(self);
}

void start_eg(envelope_generator *self)
{
    if (self->m_b_legato_mode
            && self->m_u_state != OFF
            && self->m_u_state != RELEASE)
        return;
    reset(self);
    self->m_u_state = ATTACK;
}

void stop_eg(envelope_generator *self)
{
    self->m_u_state = OFF;
}

double do_envelope(envelope_generator *self, double *p_biased_output)
{
    switch(self->m_u_state)
    {
        case OFF:
        {
            if ( self->m_b_reset_to_zero )
                self->m_d_envelope_output = 0.0;
            break;
        }
        case ATTACK:
        {
            self->m_d_envelope_output = self->m_d_attack_offset
                                        + self->m_d_envelope_output
                                        * self->m_d_attack_coeff;
            if (self->m_d_envelope_output >= 1.0
                    || self->m_d_attack_time_msec <= 0.0)
            {
                self->m_d_envelope_output = 1.0;
                self->m_u_state = DECAY;
            }
            break;
        }
        case DECAY:
        {
            self->m_d_envelope_output = self->m_d_decay_offset
                                        + self->m_d_envelope_output
                                        * self->m_d_decay_coeff;
            if (self->m_d_envelope_output <= self->m_d_sustain_level
                    || self->m_d_decay_time_msec <= 1.0)
            {
                self->m_d_envelope_output = self->m_d_sustain_level;
                self->m_u_state = SUSTAIN;
            }
            break;
        }
        case SUSTAIN:
        {
            self->m_d_envelope_output = self->m_d_sustain_level;
            break;
        }
        case RELEASE:
        {
            self->m_d_envelope_output = self->m_d_release_offset
                                        + self->m_d_envelope_output
                                        * self->m_d_release_coeff;
            if (self->m_d_envelope_output <= 0.0
                    || self->m_d_release_time_msec <= 0.0)
            {
                self->m_d_envelope_output = 0.0;
                self->m_u_state = OFF;
            }
            break;
        }
        case SHUTDOWN:
        {
            if (self->m_b_reset_to_zero)
            {
                self->m_d_envelope_output += self->m_d_inc_shutdown;
                if(self->m_d_envelope_output <= 0)
                {
                    self->m_u_state = OFF;
                    self->m_d_envelope_output = 0.0;
                    break;
                }
            }
            else
            {
                self->m_u_state = OFF;
            }
            break;
        }
    }
    if (p_biased_output)
        *p_biased_output = self->m_d_envelope_output - self->m_d_sustain_level;

    return self->m_d_envelope_output;
}
                    


