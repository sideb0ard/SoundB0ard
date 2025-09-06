#pragma once

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"

struct MoogLadder : public Filter {
  MoogLadder();
  ~MoogLadder() override = default;

  double m_k{0};
  double m_gamma{0};
  double m_alpha_0{1.0};

  double m_a{0};
  double m_b{0};
  double m_c{0};
  double m_d{0};
  double m_e{0};

  OnePole m_LPF1;
  OnePole m_LPF2;
  OnePole m_LPF3;
  OnePole m_LPF4;

  void SetQControl(double qcontrol) override;
  void Reset() override;
  void Update() override;
  double DoFilter(double xn) override;
};
