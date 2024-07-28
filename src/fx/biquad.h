#pragma once

#define FLT_MIN_PLUS 1.175494351e-38   /* min positive value */
#define FLT_MIN_MINUS -1.175494351e-38 /* min negative value */

struct Biquad {
  Biquad() = default;
  ~Biquad() = default;
  double m_xz_1{0};
  double m_xz_2{0};
  double m_yz_1{0};
  double m_yz_2{0};

  double m_a0{0};
  double m_a1{0};
  double m_a2{0};
  double m_b1{0};
  double m_b2{0};

  void FlushDelays();
  double Process(double xn);
};
