#pragma once

#define FLT_MIN_PLUS 1.175494351e-38   /* min positive value */
#define FLT_MIN_MINUS -1.175494351e-38 /* min negative value */

struct Biquad {
  Biquad() = default;
  ~Biquad() = default;
  double m_xz_1;
  double m_xz_2;
  double m_yz_1;
  double m_yz_2;

  double m_a0;
  double m_a1;
  double m_a2;
  double m_b1;
  double m_b2;

  void FlushDelays();
  double Process(double xn);
};
