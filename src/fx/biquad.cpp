#include <fx/biquad.h>
#include <stdio.h>

void Biquad::FlushDelays() {
  m_xz_1 = 0;
  m_xz_2 = 0;
  m_yz_1 = 0;
  m_yz_2 = 0;
}

double Biquad::Process(double xn) {
  double yn =
      m_a0 * xn + m_a1 * m_xz_1 + m_a2 * m_xz_2 - m_b1 * m_yz_1 - m_b2 * m_yz_2;

  if (yn > 0.0 && yn < FLT_MIN_PLUS) yn = 0;
  if (yn < 0.0 && yn > FLT_MIN_MINUS) yn = 0;

  m_yz_2 = m_yz_1;
  m_yz_1 = yn;

  m_xz_2 = m_xz_1;
  m_xz_1 = xn;

  return yn;
}
