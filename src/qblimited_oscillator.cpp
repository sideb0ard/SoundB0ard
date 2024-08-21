#include "qblimited_oscillator.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "lookuptables.h"
#include "utils.h"

double QBLimitedOscillator::DoOscillate(double *aux_output) {
  if (!m_note_on) return 0.0;

  double out = 0.0;

  bool wrap = CheckWrapModulo();

  double calc_modulo = m_modulo + m_phase_mod;
  CheckWrapIndex(&calc_modulo);

  switch (m_waveform) {
    case SINE: {
      double angle = calc_modulo * 2.0 * (double)M_PI - (double)M_PI;
      out = parabolic_sine(-1.0 * angle, true);
      break;
    }

    case SAW1:
    case SAW2:
    case SAW3: {
      out = DoSawtooth(calc_modulo, m_inc);
      break;
    }

    case SQUARE: {
      out = DoSquare(calc_modulo, m_inc);
      break;
    }

    case TRI: {
      if (wrap) m_dpw_square_modulator *= -1.0;

      out = DoTriangle(calc_modulo, m_inc, m_fo, m_dpw_square_modulator,
                       &m_dpw_z1);
      break;
    }

    case NOISE: {
      out = do_white_noise();
      break;
    }

    case PNOISE: {
      out = do_pn_sequence(&m_pn_register);
      break;
    }
    default:
      break;
  }

  IncModulo();
  if (m_waveform == TRI) IncModulo();

  if (modmatrix) {
    modmatrix->sources[m_mod_dest_output1] = out * m_amplitude * m_amp_mod;
    modmatrix->sources[m_mod_dest_output2] = out * m_amplitude * m_amp_mod;
  }

  if (aux_output) *aux_output = out * m_amplitude * m_amp_mod;

  return out * m_amplitude * m_amp_mod;
}

inline void QBLimitedOscillator::Reset() {
  Oscillator::Reset();
  if (m_waveform == SAW1 || m_waveform == SAW2 || m_waveform == SAW3 ||
      m_waveform == TRI)
    m_modulo = 0.5;
}
void QBLimitedOscillator::StartOscillator() {
  Oscillator::Reset();
  m_note_on = true;
}

void QBLimitedOscillator::StopOscillator() { m_note_on = false; }

double QBLimitedOscillator::DoSawtooth(double modulo, double inc) {
  double trivial_saw = 0.0;
  double out = 0.0;

  if (m_waveform == SAW1) {
    trivial_saw = unipolar_to_bipolar(modulo);
  } else if (m_waveform == SAW2)
    trivial_saw = 2.0 * (tanh(1.5 * modulo) / tanh(1.5)) - 1.0;
  else if (m_waveform == SAW3) {
    trivial_saw = unipolar_to_bipolar(modulo);
    trivial_saw = tanh(1.5 * trivial_saw) / tanh(1.5);
  }

  // --- NOTE: Fs/8 = Nyquist/4
  if (m_fo <= SAMPLE_RATE / 8.0) {
    out = trivial_saw + do_blep_n(&blep_table_8_blkhar[0], blep_table_size,
                                  modulo,    /* current phase value */
                                  fabs(inc), /* abs(inc) is for FM
                                                 synthesis with negative
                                                 frequencies */
                                  1.0,       /* sawtooth edge height = 1.0 */
                                  false,     /* falling edge */
                                  4,         /* 1 point per side */
                                  false);    /* interpolation */
  } else  // to prevent overlapM_PIng BLEPs, default back to 2-point for f >
          // Nyquist/4
  {
    out = trivial_saw + do_blep_n(&blep_table[0], blep_table_size,
                                  modulo,    /* current phase value */
                                  fabs(inc), /* abs(inc) is for FM
                                                 synthesis with negative
                                                 frequencies */
                                  1.0,       /* sawtooth edge height = 1.0 */
                                  false,     /* falling edge */
                                  1,         /* 1 point per side */
                                  true);     /* interpolation */
  }

  return out;
}

inline double QBLimitedOscillator::DoSquare(double modulo, double inc) {
  m_waveform = SAW1;
  double dSaw1 = DoSawtooth(modulo, inc);
  if (inc > 0)
    modulo += m_pulse_width / 100.0;
  else
    modulo -= m_pulse_width / 100.0;

  if (inc > 0 && modulo >= 1.0) modulo -= 1.0;

  if (inc < 0 && modulo <= 0.0) modulo += 1.0;

  double dSaw2 = DoSawtooth(modulo, inc);
  double out = 0.5 * dSaw1 - 0.5 * dSaw2;
  double dCorr = 1.0 / (m_pulse_width / 100.0);

  if ((m_pulse_width / 100.0) < 0.5)
    dCorr = 1.0 / (1.0 - (m_pulse_width / 100.0));

  out *= dCorr;
  m_waveform = SQUARE;

  return out;
}

double QBLimitedOscillator::DoTriangle(double modulo, double inc, double fo,
                                       double square_modulator,
                                       double *z_register) {
  double bipolar = unipolar_to_bipolar(modulo);
  double sq = bipolar * bipolar;
  double inv = 1.0 - sq;
  double sq_mod = inv * square_modulator;
  double differentiated_sq_mod = sq_mod - *z_register;
  *z_register = sq_mod;
  double c = SAMPLE_RATE / (4.0 * 2.0 * fo * (1 - inc));

  return differentiated_sq_mod * c;
}
