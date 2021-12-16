#include "wt_oscillator.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "defjams.h"
#include "utils.h"

WTOscillator::WTOscillator() {
  memset(m_saw_tables, 0, kNumTables * sizeof(double));
  memset(m_tri_tables, 0, kNumTables * sizeof(double));

  m_read_idx = 0;
  m_wt_inc = 0;
  m_read_idx2 = 0;
  m_wt_inc2 = 0;
  m_current_table_idx = 0;

  m_current_table = &m_sine_table[0];

  m_square_corr_factor[0] = 0.5;
  m_square_corr_factor[1] = 0.5;
  m_square_corr_factor[2] = 0.5;
  m_square_corr_factor[3] = 0.49;
  m_square_corr_factor[4] = 0.48;
  m_square_corr_factor[5] = 0.468;
  m_square_corr_factor[6] = 0.43;
  m_square_corr_factor[7] = 0.34;
  m_square_corr_factor[8] = 0.25;

  CreateWaveTables();
  std::cout << "WAVE TABLES CREATED\n";
}

void WTOscillator::Reset() {
  Oscillator::Reset();
  m_read_idx = 0;
  m_read_idx2 = 0;

  Update();
}

double WTOscillator::DoOscillate(double *quad_outval) {
  if (!m_note_on) {
    if (quad_outval) *quad_outval = 0.0;

    return 0.0;
  }

  if (m_waveform == SQUARE && m_current_table_idx >= 0) {
    double out = DoSquareWave();
    if (quad_outval) *quad_outval = out;
    return out;
  }

  double outval = DoWaveTable(&m_read_idx, m_wt_inc);
  if (modmatrix) {
    modmatrix->sources[m_mod_dest_output1] = outval * m_amplitude * m_amp_mod;
    modmatrix->sources[m_mod_dest_output2] = outval * m_amplitude * m_amp_mod;
  }

  if (quad_outval) *quad_outval = outval * m_amplitude * m_amp_mod;

  return outval * m_amplitude * m_amp_mod;
}

double WTOscillator::DoWaveTable(double *read_idx, double wt_inc) {
  double out = 0.;
  double mod_read_idx = *read_idx + m_phase_mod * kWavetableLength;
  CheckWrapIndex(&mod_read_idx);

  int i_read_idx = abs((int)mod_read_idx);
  float frac = mod_read_idx - i_read_idx;
  int read_idx_next =
      i_read_idx + 1 > kWavetableLength - 1 ? 0 : i_read_idx + 1;
  out = utils::LinTerp(0, 1, m_current_table[i_read_idx],
                       m_current_table[read_idx_next], frac);
  *read_idx += wt_inc;

  CheckWrapIndex(read_idx);
  return out;
}

double WTOscillator::DoSquareWave() {
  return DoSquareWaveCore(&m_read_idx, m_wt_inc);
}

double WTOscillator::DoSquareWaveCore(double *read_idx, double wt_inc) {
  double pw = m_pulse_width / 100;
  double pwidx = *read_idx + pw * kWavetableLength;

  double saw1 = DoWaveTable(read_idx, wt_inc);

  if (wt_inc >= 0) {
    if (pwidx >= kWavetableLength) pwidx = pwidx - kWavetableLength;
  } else {
    if (pwidx < 0) pwidx = pwidx + kWavetableLength;
  }

  double saw2 = DoWaveTable(&pwidx, wt_inc);

  double sqamp = m_square_corr_factor[m_current_table_idx];
  double out = sqamp * saw1 - sqamp * saw2;

  double corr = 1.0 / pw;
  if (pw < 0.5) corr = 1.0 / (1.0 - pw);

  out *= corr;
  return out;
}

void WTOscillator::StartOscillator() { m_note_on = true; }

void WTOscillator::StopOscillator() { m_note_on = false; }

void WTOscillator::Update() {
  Oscillator::Update();
  m_wt_inc = (double)kWavetableLength * m_inc;
  SelectTable();
}

void WTOscillator::SelectTable() {
  m_current_table_idx = GetTableIndex();
  if (m_current_table_idx < 0) {
    m_current_table = m_sine_table;
    return;
  }

  if (m_waveform == SAW1 || m_waveform == SAW2 || m_waveform == SAW3 ||
      m_waveform == SQUARE)
    m_current_table = m_saw_tables[m_current_table_idx];
  else if (m_waveform == TRI)
    m_current_table = m_tri_tables[m_current_table_idx];
}

int WTOscillator::GetTableIndex() {
  if (m_waveform == SINE) return -1;

  double seed_freq = 27.5;  // A0
  for (int i = 0; i < kNumTables; ++i) {
    if (m_fo <= seed_freq) return i;

    seed_freq *= 2.0;
  }
  return -1;
}

void WTOscillator::CreateWaveTables() {
  for (int i = 0; i < kWavetableLength; i++)
    m_sine_table[i] = sin((static_cast<double>(i) / kWavetableLength) * TWO_PI);

  double seed_freq = 27.5;  // A0
  for (int i = 0; i < kNumTables; ++i) {
    double *saw_table = new double[kWavetableLength];
    memset(saw_table, 0, kWavetableLength * sizeof(double));
    double *tri_table = new double[kWavetableLength];
    memset(tri_table, 0, kWavetableLength * sizeof(double));

    int harms = (int)((SAMPLE_RATE / 2.0 / seed_freq) - 1.0);
    int half_harms = (int)((float)harms / 2.0);
    double max_saw = 0;
    double max_tri = 0;

    for (int j = 0; j < kWavetableLength; ++j) {
      for (int g = 1; g <= harms; ++g) {
        double x = g * M_PI / harms;
        double sigma = sin(x) / x;
        if (g == 1) sigma = 1.0;
        double n = g;
        saw_table[j] += pow((float)-1.0, (float)(g + 1)) * (1.0 / n) * sigma *
                        sin(TWO_PI * j * n / kWavetableLength);
      }
      for (int g = 1; g <= half_harms; ++g) {
        double n = g;
        tri_table[j] += pow((float)-1.0, (float)n) *
                        (1.0 / pow((float)(2 * n + 1), (float)2.0)) *
                        sin(TWO_PI * (2.0 * n + 1) * j / kWavetableLength);
      }

      if (j == 0) {
        max_saw = saw_table[j];
        max_tri = tri_table[j];
      } else {
        if (saw_table[j] > max_saw) max_saw = saw_table[j];
        if (tri_table[j] > max_tri) max_tri = tri_table[j];
      }
    }

    // normalize
    for (int j = 0; j < kWavetableLength; ++j) {
      saw_table[j] /= max_saw;
      tri_table[j] /= max_tri;
    }

    m_saw_tables[i] = saw_table;
    m_tri_tables[i] = tri_table;
    seed_freq *= 2.0;
  }
}

inline void wt_check_wrap_index(double *index) {
  while (*index < 0.0) *index += kWavetableLength;
  while (*index >= kWavetableLength) *index -= kWavetableLength;
}
