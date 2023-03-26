#include "oscillator.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>

#include "modmatrix.h"
#include "soundgenerator.h"
#include "utils.h"

Oscillator::Oscillator() { m_pn_register = rand(); }

void Oscillator::IncModulo() { m_modulo += m_inc; }

bool Oscillator::CheckWrapModulo() {
  if (m_inc > 0 && m_modulo >= 1.0) {
    m_modulo -= 1.0;
    return true;
  }
  if (m_inc < 0 && m_modulo <= 0.0) {
    m_modulo += 1.0;
    return true;
  }
  return false;
}

void Oscillator::ResetModulo(double d) { m_modulo = d; }

void Oscillator::SetAmplitudeMod(double amp_val) { m_amp_mod = amp_val; }

void Oscillator::SetFoModExp(double fo_mod_val) { m_fo_mod = fo_mod_val; }

void Oscillator::SetPitchBendMod(double mod_val) { m_pitch_bend_mod = mod_val; }
void Oscillator::SetFoModLin(double fo_mod_val) { m_fo_mod_lin = fo_mod_val; }

void Oscillator::SetPhaseMod(double mod_val) { m_phase_mod = mod_val; }

void Oscillator::SetPwMod(double mod_val) { m_pw_mod = mod_val; }

void Oscillator::Reset() {
  m_modulo = 0.0;
  m_dpw_square_modulator = -1.0;
  m_dpw_z1 = 0.0;

  m_pn_register = rand();
  m_rsh_counter = -1;  // flag for reset condition
  m_rsh_value = 0.0;

  m_amp_mod = 1.0;  // note default to 1 to avoid silent osc
  m_pw_mod = 0.0;
  m_pitch_bend_mod = 0.0;
  m_fo_mod = 0.0;
  m_fo_mod_lin = 0.0;
  m_phase_mod = 0.0;
}

void Oscillator::Update() {
  if (global_oscillator_params) {
    if (global_oscillator_params->osc_fo >= 0)
      m_osc_fo = global_oscillator_params->osc_fo;

    m_fo_ratio = global_oscillator_params->fo_ratio;
    m_amplitude = global_oscillator_params->amplitude;
    m_pulse_width_control = global_oscillator_params->pulse_width_control;
    m_octave = global_oscillator_params->octave;
    m_semitones = global_oscillator_params->semitones;
    m_cents = global_oscillator_params->cents;
    m_waveform = global_oscillator_params->waveform;
    m_lfo_mode = global_oscillator_params->lfo_mode;
  }

  // --- ignore LFO mode for noise sources
  if (m_waveform == rsh || m_waveform == qrsh) m_lfo_mode = LFOFREE;

  // --- get from matrix Sources
  if (modmatrix) {
    // --- zero is norm for these
    m_fo_mod = modmatrix->destinations[m_mod_source_fo];

    m_pw_mod = modmatrix->destinations[m_mod_source_pulse_width];

    // --- amp mod is 0->1
    // --- invert for oscillator output mod
    m_amp_mod = modmatrix->destinations[m_mod_source_amp];
    m_amp_mod = 1.0 - m_amp_mod;
  }

  // --- do the  complete frequency mod
  m_fo = m_osc_fo * m_fo_ratio *
         pitch_shift_multiplier(m_fo_mod + m_pitch_bend_mod + m_octave * 12.0 +
                                m_semitones + m_cents / 100.0);

  // --- apply linear FM (not used in book projects)
  m_fo += m_fo_mod_lin;

  // --- bound Fo (can go outside for FM/PM mod)
  //     +/- 20480 for FM/PM
  if (m_fo > OSC_FO_MAX) m_fo = OSC_FO_MAX;
  if (m_fo < -OSC_FO_MAX) m_fo = -OSC_FO_MAX;

  m_inc = m_fo / SAMPLE_RATE;

  // --- limits are 2% and 98%
  m_pulse_width =
      m_pulse_width_control +
      m_pw_mod * (OSC_PULSEWIDTH_MAX - OSC_PULSEWIDTH_MIN) / OSC_PULSEWIDTH_MIN;

  // --- bound the PWM to the range
  m_pulse_width = fmin(m_pulse_width, OSC_PULSEWIDTH_MAX);
  m_pulse_width = fmax(m_pulse_width, OSC_PULSEWIDTH_MIN);
}

void Oscillator::InitGlobalParameters(GlobalOscillatorParams *params) {
  global_oscillator_params = params;
  global_oscillator_params->osc_fo = -1.0;
  global_oscillator_params->fo_ratio = m_fo_ratio;
  global_oscillator_params->amplitude = m_amplitude;
  global_oscillator_params->pulse_width_control = m_pulse_width_control;
  global_oscillator_params->octave = m_octave;
  global_oscillator_params->semitones = m_semitones;
  global_oscillator_params->cents = m_cents;
  global_oscillator_params->waveform = m_waveform;
  global_oscillator_params->lfo_mode = m_lfo_mode;
}
