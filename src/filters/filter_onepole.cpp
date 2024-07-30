#include "filter_onepole.h"

#include <math.h>
#include <stdlib.h>

#include <iostream>

#include "defjams.h"

OnePole::OnePole() {
  m_filter_type = LPF1;
  Reset();
}

void OnePole::Update() {
  Filter::Update();
  double wd = TWO_PI * m_fc;
  double T = 1.0 / SAMPLE_RATE;
  double wa = (2.0 / T) * tan(wd * T / 2.0);
  double g = wa * T / 2.0;

  m_alpha = g / (1.0 + g);
}

void OnePole::SetFeedback(double fb) { m_feedback = fb; }

double OnePole::GetFeedbackOutput() {
  return m_beta * (m_z1 + m_feedback * m_delta);
}

void OnePole::Reset() {
  m_z1 = 0.0;
  m_feedback = 0.0;
}

double OnePole::DoFilter(double xn) {
  if (m_filter_type != LPF1 && m_filter_type != HPF1) {
    std::cerr << "OnePole filter - wrong type!\n";
    return xn;
  }

  xn = xn * m_gamma + m_feedback + m_epsilon * GetFeedbackOutput();

  double vn = (m_a0 * xn - m_z1) * m_alpha;

  double lpf = vn + m_z1;

  m_z1 = vn + lpf;

  double hpf = xn - lpf;

  if (m_filter_type == LPF1)
    return lpf;
  else if (m_filter_type == HPF1)
    return hpf;

  std::cerr << "SHOULD NOT REACH\n";
  return xn;
}
