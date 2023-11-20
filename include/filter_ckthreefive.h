#pragma once

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"

struct CKThreeFive : public Filter {
  CKThreeFive();
  ~CKThreeFive() = default;
  double m_k{0.01};
  double m_alpha0{0};

  OnePole m_LPF1{LPF1};
  OnePole m_LPF2{LPF1};
  OnePole m_HPF1{HPF1};
  OnePole m_HPF2{HPF1};

  void Update() override;
  void SetQControl(double qcontrol) override;
  void Reset() override;
  double DoFilter(double xn) override;
};
