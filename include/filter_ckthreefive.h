#pragma once

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"

struct CKThreeFive : public Filter {
  CKThreeFive() = default;
  ~CKThreeFive() = default;
  double m_k{0.1};
  double m_alpha0{0};

  unsigned int m_filter_type{LPF2};

  OnePole m_LPF1{LPF1};
  OnePole m_LPF2{LPF2};
  OnePole m_HPF1{HPF1};
  OnePole m_HPF2{HPF2};

  void Update() override;
  void SetQControl(double qcontrol) override;
  void Reset() override;
  double DoFilter(double xn) override;
};
