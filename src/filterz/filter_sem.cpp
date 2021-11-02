#include "filter_sem.h"

#include <math.h>
#include <stdlib.h>

#include <iostream>

#include "defjams.h"

FilterSem::FilterSem() {
  m_aux_control = 0.5;
  m_filter_type = LPF2;

  m_alpha = 1.0;
  m_alpha0 = 1.0;
  m_rho = 1.0;

  Reset();
}

void FilterSem::Reset() {
  m_z11 = 0;
  m_z12 = 0;
}

void FilterSem::SetQControl(double qcontrol) {
  m_q = (25.0 - 0.5) * (qcontrol - 1.0) / (10.0 - 1.0) + 0.5;
}

void FilterSem::Update() {
  Filter::Update();  // update base class

  double wd = 2 * M_PI * m_fc;
  double T = 1.0 / SAMPLE_RATE;
  double wa = (2 / T) * tan(wd * T / 2);
  double g = wa * T / 2;
  double R = 1.0 / (2.0 * m_q);
  m_alpha0 = 1.0 / (1.0 + 2.0 * R * g + g * g);
  m_alpha = g;
  m_rho = 2.0 * R + g;
}

double FilterSem::DoFilter(double xn) {
  if (m_filter_type != LPF2 && m_filter_type != HPF2 && m_filter_type != BPF2 &&
      m_filter_type != BSF2) {
    std::cerr << "SORRY BUD, NOT SUPPORTED!\n";
    return xn;
  }

  double hpf = m_alpha0 * (xn - m_rho * m_z11 - m_z12);

  double bpf = m_alpha * hpf + m_z11;

  if (m_nlp) bpf = tanh(m_saturation * bpf);

  double lpf = m_alpha * bpf + m_z12;

  // double R = 1.0 / (2.0 * m_q);

  // double bsf = xn - 2.0 * R * bpf; // hmm, this is unused - mistake?

  double semBSF = m_aux_control * hpf + (1.0 - m_aux_control) * lpf;

  m_z11 = m_alpha * hpf + bpf;
  m_z12 = m_alpha * bpf + lpf;

  if (m_filter_type == LPF2)
    return lpf;
  else if (m_filter_type == HPF2)
    return hpf;
  else if (m_filter_type == BPF2)
    return bpf;
  else if (m_filter_type == BSF2)
    return semBSF;

  // shouldn't get here
  return xn;
}
