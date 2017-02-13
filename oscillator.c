#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "modmatrix.h"
#include "oscillator.h"
#include "sound_generator.h"
#include "utils.h"

void osc_new_settings(oscillator *osc)
{
    osc->m_note_on = false;
    osc->m_midi_note_number = 0;
    osc->m_modulo = 0.0;
    osc->m_inc = 0.0;
    osc->m_osc_fo = OSC_FO_DEFAULT; // GUI
    osc->m_amplitude = 1.0;         // default ON
    osc->m_pulse_width = OSC_PULSEWIDTH_DEFAULT;
    osc->m_pulse_width_control = OSC_PULSEWIDTH_DEFAULT; // GUI
    osc->m_fo = OSC_FO_DEFAULT;

    // --- seed the random number generator
    srand(time(NULL));
    osc->m_pn_register = rand();

    // --- continue inits
    osc->m_rsh_counter = -1; // flag for reset condition
    osc->m_rsh_value = 0.0;
    osc->m_amp_mod = 1.0; // note default to 1 to avoid silent osc
    osc->m_fo_mod_lin = 0.0;
    osc->m_phase_mod = 0.0;
    osc->m_fo_mod = 0.0;
    osc->m_pitch_bend_mod = 0.0;
    osc->m_pw_mod = 0.0;
    osc->m_octave = 0.0;
    osc->m_semitones = 0.0;
    osc->m_cents = 0.0;
    osc->m_fo_ratio = 1.0;
    osc->m_lfo_mode = 0;

    // --- pitched
    osc->m_waveform = SINE;

    // --- default modulation matrix inits
    osc->g_modmatrix = NULL;

    // --- everything is disconnected unless you use mod matrix
    osc->m_mod_source_fo = DEST_NONE;
    osc->m_mod_source_pulse_width = DEST_NONE;
    osc->m_mod_source_amp = DEST_NONE;
    osc->m_mod_dest_output1 = SOURCE_NONE;
    osc->m_mod_dest_output2 = SOURCE_NONE;

    osc->m_square_edge_rising = false;

    // --- for hard sync
    osc->m_buddy_oscillator = NULL;
    osc->m_master_osc = false;

    osc->m_global_oscillator_params = NULL;
}

// --- modulo functions for master/slave operation
// --- increment the modulo counters
void osc_inc_modulo(oscillator *self) { self->m_modulo += self->m_inc; }

// --- check and wrap the modulo
//     returns true if modulo wrapped
bool osc_check_wrap_modulo(oscillator *self)
{
    if (self->m_inc > 0 && self->m_modulo >= 1.0) {
        self->m_modulo -= 1.0;
        return true;
    }
    if (self->m_inc < 0 && self->m_modulo <= 0.0) {
        self->m_modulo += 1.0;
        return true;
    }
    return false;
}

// --- reset the modulo (required for master->slave operations)
void osc_reset_modulo(oscillator *self, double d) { self->m_modulo = d; }

void osc_set_amplitude_mod(oscillator *self, double amp_val)
{
    self->m_amp_mod = amp_val;
}

void osc_set_fo_mod_exp(oscillator *self, double fo_mod_val)
{
    self->m_fo_mod = fo_mod_val;
}

void osc_set_pitch_bend_mod(oscillator *self, double mod_val)
{
    self->m_pitch_bend_mod = mod_val;
}
void osc_set_fo_mod_lin(oscillator *self, double fo_mod_val)
{
    self->m_fo_mod_lin = fo_mod_val;
}

void osc_set_phase_mod(oscillator *self, double mod_val)
{
    self->m_phase_mod = mod_val;
}

void osc_set_pw_mod(oscillator *self, double mod_val)
{
    self->m_pw_mod = mod_val;
}

void osc_reset(oscillator *self)
{
    // --- Pitched modulos, wavetables start at 0.0
    self->m_modulo = 0.0;

    // --- needed fror triangle algorithm, DPW
    self->m_dpw_square_modulator = -1.0;

    // --- flush DPW registers
    self->m_dpw_z1 = 0.0;

    // --- for random stuff
    srand(time(NULL));
    self->m_pn_register = rand();
    self->m_rsh_counter = -1; // flag for reset condition
    self->m_rsh_value = 0.0;

    // square state variable
    self->m_square_edge_rising = false;

    // modulation variables
    self->m_amp_mod = 1.0; // note default to 1 to avoid silent osc
    self->m_pw_mod = 0.0;
    self->m_pitch_bend_mod = 0.0;
    self->m_fo_mod = 0.0;
    self->m_fo_mod_lin = 0.0;
    self->m_phase_mod = 0.0;
}

// --- update the frequency, amp mod and PWM
void osc_update(oscillator *self)
{
    if (self->m_global_oscillator_params) {
        if (self->m_global_oscillator_params->osc_fo >= 0)
            self->m_osc_fo = self->m_global_oscillator_params->osc_fo;

        self->m_fo_ratio = self->m_global_oscillator_params->fo_ratio;
        self->m_amplitude = self->m_global_oscillator_params->amplitude;
        self->m_pulse_width_control =
            self->m_global_oscillator_params->pulse_width_control;
        self->m_octave = self->m_global_oscillator_params->octave;
        self->m_semitones = self->m_global_oscillator_params->semitones;
        self->m_cents = self->m_global_oscillator_params->cents;
        self->m_waveform = self->m_global_oscillator_params->waveform;
        self->m_lfo_mode = self->m_global_oscillator_params->lfo_mode;
    }

    // --- ignore LFO mode for noise sources
    if (self->m_waveform == rsh || self->m_waveform == qrsh)
        self->m_lfo_mode = rfree;

    // --- Modulation Matrix
    //
    // --- get from matrix Sources
    if (self->g_modmatrix) {
        // --- zero is norm for these
        self->m_fo_mod =
            self->g_modmatrix->m_destinations[self->m_mod_source_fo];

        self->m_pw_mod =
            self->g_modmatrix->m_destinations[self->m_mod_source_pulse_width];

        // --- amp mod is 0->1
        // --- invert for oscillator output mod
        self->m_amp_mod =
            self->g_modmatrix->m_destinations[self->m_mod_source_amp];
        self->m_amp_mod = 1.0 - self->m_amp_mod;
    }

    // --- do the  complete frequency mod
    self->m_fo =
        self->m_osc_fo * self->m_fo_ratio *
        pitch_shift_multiplier(self->m_fo_mod + self->m_pitch_bend_mod +
                               self->m_octave * 12.0 + self->m_semitones +
                               self->m_cents / 100.0);
    // --- apply linear FM (not used in book projects)
    self->m_fo += self->m_fo_mod_lin;

    // --- bound Fo (can go outside for FM/PM mod)
    //     +/- 20480 for FM/PM
    if (self->m_fo > OSC_FO_MAX)
        self->m_fo = OSC_FO_MAX;
    if (self->m_fo < -OSC_FO_MAX)
        self->m_fo = -OSC_FO_MAX;

    // --- calculate increment (a.k.a. phase a.k.a. phaseIncrement, etc...)
    self->m_inc = self->m_fo / SAMPLE_RATE;

    // --- Pulse Width Modulation --- //

    // --- limits are 2% and 98%
    self->m_pulse_width = self->m_pulse_width_control +
                          self->m_pw_mod *
                              (OSC_PULSEWIDTH_MAX - OSC_PULSEWIDTH_MIN) /
                              OSC_PULSEWIDTH_MIN;

    // --- bound the PWM to the range
    self->m_pulse_width = fmin(self->m_pulse_width, OSC_PULSEWIDTH_MAX);
    self->m_pulse_width = fmax(self->m_pulse_width, OSC_PULSEWIDTH_MIN);
}

void osc_init_global_parameters(oscillator *self,
                                global_oscillator_params *params)
{
    self->m_global_oscillator_params = params;
    self->m_global_oscillator_params->osc_fo = self->m_osc_fo;
}
