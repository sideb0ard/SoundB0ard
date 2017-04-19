#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "envelope_generator.h"
// this ordering should match the enum defined in envelope_header.h
//
char *state_strings[] = {"OFFF",    "ATTACK",  "DECAY",
                         "SUSTAIN", "RELEASE", "SHUTDOWN"};

envelope_generator *new_envelope_generator()
{
    envelope_generator *eg =
        (envelope_generator *)calloc(1, sizeof(envelope_generator));

    if (eg == NULL)
        return NULL;

    envelope_generator_init(eg);

    return eg;
}

void envelope_generator_init(envelope_generator *eg)
{
    eg->m_attack_time_msec = EG_DEFAULT_STATE_TIME;
    eg->m_decay_time_msec = EG_DEFAULT_STATE_TIME;
    eg->m_release_time_msec = EG_DEFAULT_STATE_TIME;

    eg->m_attack_time_scalar = 1.0;
    eg->m_decay_time_scalar = 1.0;

    eg->m_sustain_level = 0.7;
    eg->m_envelope_output = 0.0;

    eg->m_inc_shutdown = 0.0;

    eg->m_shutdown_time_msec = 10.0;

    eg->m_state = OFFF;
    eg->m_sustain_override = false;
    eg->m_release_pending = false;
    eg->m_output_eg = false;
    eg_set_eg_mode(eg, ANALOG);
    eg->m_reset_to_zero = false;
    eg->m_legato_mode = false;

    eg->m_v_modmatrix = NULL;

    eg->m_mod_dest_eg_output = SOURCE_NONE;
    eg->m_mod_dest_eg_biased_output = SOURCE_NONE;

    eg->m_mod_source_eg_attack_scaling = DEST_NONE;
    eg->m_mod_source_eg_decay_scaling = DEST_NONE;
    eg->m_mod_source_sustain_override = DEST_NONE;

    eg->m_global_eg_params = NULL;
}

unsigned int eg_get_state(envelope_generator *self) { return self->m_state; }

bool eg_is_active(envelope_generator *self)
{
    if (self->m_state != RELEASE && self->m_state != OFFF &&
        !self->m_release_pending)
        return true;
    return false;
}

bool eg_can_note_off(envelope_generator *self)
{
    if (self->m_state != RELEASE && self->m_state != SHUTDOWN &&
        self->m_state != OFFF && !self->m_release_pending)
        return true;
    return false;
}

void eg_reset(envelope_generator *self)
{
    self->m_attack_time_scalar = 1.0;
    self->m_decay_time_scalar = 1.0;
    self->m_state = OFFF;
    self->m_release_pending = false;
    eg_set_eg_mode(self, self->m_eg_mode);

    eg_calculate_release_time(self);

    if (self->m_reset_to_zero) {
        self->m_envelope_output = 0.0;
    }
}

void eg_set_eg_mode(envelope_generator *self, unsigned int mode)
{
    self->m_eg_mode = mode;
    if (self->m_eg_mode == ANALOG) {
        self->m_attack_tco = exp(-0.5); // fast attack
        // self->m_attack_tco = exp(-5.0);
        self->m_decay_tco = exp(-5.0);
        self->m_release_tco = self->m_decay_tco;
    }
    else {
        self->m_attack_tco = pow(10.0, -96.0 / 20.0);
        self->m_decay_tco = self->m_attack_tco;
        self->m_release_tco = self->m_decay_tco;
    }

    eg_calculate_attack_time(self);
    eg_calculate_decay_time(self);
    eg_calculate_release_time(self);
}

void eg_calculate_attack_time(envelope_generator *self)
{
    double d_samples =
        SAMPLE_RATE *
        ((self->m_attack_time_scalar * self->m_attack_time_msec) / 1000.0);
    self->m_attack_coeff =
        exp(-log((1.0 + self->m_attack_tco) / self->m_attack_tco) / d_samples);
    self->m_attack_offset =
        (1.0 + self->m_attack_tco) * (1.0 - self->m_attack_coeff);
}

void eg_calculate_decay_time(envelope_generator *self)
{
    double d_samples =
        SAMPLE_RATE *
        ((self->m_decay_time_scalar * self->m_decay_time_msec) / 1000.0);
    self->m_decay_coeff =
        exp(-log((1.0 + self->m_decay_tco) / self->m_decay_tco) / d_samples);
    self->m_decay_offset = (self->m_sustain_level - self->m_decay_tco) *
                           (1.0 - self->m_decay_coeff);
}

void eg_calculate_release_time(envelope_generator *self)
{
    // printf("CALC RELEASE TIME!\n");
    double d_samples = SAMPLE_RATE * (self->m_release_time_msec / 1000.0);
    self->m_release_coeff = exp(
        -log((1.0 + self->m_release_tco) / self->m_release_tco) / d_samples);
    self->m_release_offset =
        -self->m_release_tco * (1.0 - self->m_release_coeff);
}

void eg_set_attack_time_msec(envelope_generator *self, double time)
{
    self->m_attack_time_msec = time;
    eg_calculate_attack_time(self);
}

void eg_set_decay_time_msec(envelope_generator *self, double time)
{
    self->m_decay_time_msec = time;
    eg_calculate_decay_time(self);
}

void eg_set_release_time_msec(envelope_generator *self, double time)
{
    self->m_release_time_msec = time;
    eg_calculate_release_time(self);
}

void eg_set_shutdown_time_msec(envelope_generator *self, double time)
{
    self->m_shutdown_time_msec = time;
}

void eg_set_sustain_level(envelope_generator *self, double level)
{
    self->m_sustain_level = level;
    eg_calculate_decay_time(self);
    if (self->m_state != RELEASE)
        eg_calculate_release_time(self);
}

void eg_set_sustain_override(envelope_generator *self, bool b)
{
    self->m_sustain_override = b;
    if (self->m_release_pending && !self->m_sustain_override) {
        self->m_release_pending = false;
        eg_note_off(self);
    }
}

void eg_start_eg(envelope_generator *self)
{
    if (self->m_legato_mode && self->m_state != OFFF &&
        self->m_state != RELEASE) {
        return;
    }

    // NOTE: has caused issues -
    eg_reset(self);
    self->m_state = ATTACK;
}

void eg_release(envelope_generator *self)
{
    if (self->m_state == SUSTAIN)
        self->m_state = RELEASE;
}

void eg_stop_eg(envelope_generator *self) { self->m_state = OFFF; }

void eg_init_global_parameters(envelope_generator *self,
                               global_eg_params *params)
{
    self->m_global_eg_params = params;

    self->m_global_eg_params->attack_time_msec = self->m_attack_time_msec;
    self->m_global_eg_params->decay_time_msec = self->m_decay_time_msec;
    self->m_global_eg_params->release_time_msec = self->m_release_time_msec;
    self->m_global_eg_params->sustain_level = self->m_sustain_level;
    self->m_global_eg_params->shutdown_time_msec = self->m_shutdown_time_msec;
    self->m_global_eg_params->reset_to_zero = self->m_reset_to_zero;
    self->m_global_eg_params->legato_mode = self->m_legato_mode;
}

void eg_update(envelope_generator *self)
{
    if (self->m_global_eg_params) {
        if (self->m_attack_time_msec !=
            self->m_global_eg_params->attack_time_msec) {
            eg_set_attack_time_msec(self,
                                    self->m_global_eg_params->attack_time_msec);
        }

        if (self->m_decay_time_msec !=
            self->m_global_eg_params->decay_time_msec)
            eg_set_decay_time_msec(self,
                                   self->m_global_eg_params->attack_time_msec);

        if (self->m_release_time_msec !=
            self->m_global_eg_params->release_time_msec)
            eg_set_release_time_msec(
                self, self->m_global_eg_params->release_time_msec);

        if (self->m_sustain_level != self->m_global_eg_params->sustain_level)
            eg_set_sustain_level(self, self->m_global_eg_params->sustain_level);

        self->m_shutdown_time_msec =
            self->m_global_eg_params->shutdown_time_msec;
        self->m_reset_to_zero = self->m_global_eg_params->reset_to_zero;
        self->m_legato_mode = self->m_global_eg_params->legato_mode;
    }

    if (!self->m_v_modmatrix) {
        return;
    }

    // --- with mod matrix, when value is 0 there is NO modulation, so here
    if (self->m_mod_source_eg_attack_scaling != DEST_NONE &&
        self->m_attack_time_scalar == 1.0) {
        double scale =
            self->m_v_modmatrix
                ->m_destinations[self->m_mod_source_eg_attack_scaling];
        if (self->m_attack_time_scalar != 1.0 - scale) {
            self->m_attack_time_scalar = 1.0 - scale;
            eg_calculate_attack_time(self);
        }
    }

    // --- for vel->attack and note#->decay scaling modulation
    //     NOTE: make sure this is only called ONCE during a new note event!
    if (self->m_mod_source_eg_decay_scaling != DEST_NONE &&
        self->m_decay_time_scalar == 1.0) {
        double scale =
            self->m_v_modmatrix
                ->m_destinations[self->m_mod_source_eg_decay_scaling];
        if (self->m_decay_time_scalar != 1.0 - scale) {
            self->m_decay_time_scalar = 1.0 - scale;
            eg_calculate_decay_time(self);
        }
    }

    if (self->m_mod_source_sustain_override != DEST_NONE) {
        printf("SUSTAIN OVERRIDE IS NOT DEST NONE!??\n");
        double sustain =
            self->m_v_modmatrix
                ->m_destinations[self->m_mod_source_sustain_override];
        if (sustain == 0)
            eg_set_sustain_override(self, false);
        else
            eg_set_sustain_override(self, true);
    }
}

double eg_do_envelope(envelope_generator *self, double *p_biased_output)
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
        if (self->m_envelope_output >= 1.0 ||
            self->m_attack_time_scalar * self->m_attack_time_msec <= 0.0) {
            self->m_envelope_output = 1.0;
            self->m_state = DECAY;
            break;
        }
        break;
    }
    case DECAY: {
        self->m_envelope_output = self->m_decay_offset +
                                  self->m_envelope_output * self->m_decay_coeff;
        if (self->m_envelope_output <= self->m_sustain_level ||
            self->m_decay_time_scalar * self->m_decay_time_msec <= 0.0) {
            self->m_envelope_output = self->m_sustain_level;
            self->m_state = SUSTAIN;
            break;
        }
        break;
    }
    case SUSTAIN: {
        self->m_envelope_output = self->m_sustain_level;
        break;
    }
    case RELEASE: {
        if (self->m_sustain_override) {
            // printf("SUSTAIN OVERRIDE!\n");
            self->m_envelope_output = self->m_sustain_level;
            break;
        }
        else {
            // printf("ELSE: ENV OUTPUT %f\n", self->m_envelope_output);
            self->m_envelope_output =
                self->m_release_offset +
                self->m_envelope_output * self->m_release_coeff;
            // printf("else: ENV OUTPUT NOW %f offset %f release coeff %f\n",
            // self->m_envelope_output, self->m_release_offset,
            // self->m_release_coeff);
        }

        if (self->m_envelope_output <= 0.0 ||
            self->m_release_time_msec <= 0.0) {
            self->m_envelope_output = 0.0;
            self->m_state = OFFF;
            break;
        }
        break;
    }
    case SHUTDOWN: {
        if (self->m_reset_to_zero) {
            self->m_envelope_output += self->m_inc_shutdown;
            if (self->m_envelope_output <= 0) {
                self->m_envelope_output = 0.0;
                self->m_state = OFFF;
                break;
            }
        }
        else {
            self->m_state = OFFF;
        }
        break;
    }
    }

    if (self->m_v_modmatrix) {
        self->m_v_modmatrix->m_sources[self->m_mod_dest_eg_output] =
            self->m_envelope_output;
        self->m_v_modmatrix->m_sources[self->m_mod_dest_eg_biased_output] =
            self->m_envelope_output - self->m_sustain_level;
    }

    if (p_biased_output)
        *p_biased_output = self->m_envelope_output - self->m_sustain_level;

    return self->m_envelope_output;
}

void eg_note_off(envelope_generator *self)
{
    if (self->m_sustain_override) {
        self->m_release_pending = true;
        return;
    }

    if (self->m_envelope_output > 0) {
        self->m_state = RELEASE;
    }
    else {
        self->m_state = OFFF;
    }
}

void eg_shutdown(envelope_generator *self)
{
    if (self->m_legato_mode)
        return;
    self->m_inc_shutdown = -(1000.0 * self->m_envelope_output) /
                           self->m_shutdown_time_msec / SAMPLE_RATE;
    self->m_state = SHUTDOWN;
    self->m_sustain_override = false;
    self->m_release_pending = false;
}
