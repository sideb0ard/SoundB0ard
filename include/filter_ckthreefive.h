#pragma once

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"

struct CKThreeFive : public Filter {
  CKThreeFive();
  ~CKThreeFive() = default;
  double m_k;
  double m_alpha0;

  unsigned int m_filter_type;

  OnePole m_LPF1;
  OnePole m_LPF2;
  OnePole m_HPF1;
  OnePole m_HPF2;

  void Update() override;
  void SetQControl(double qcontrol) override;
  void Reset() override;
  double DoFilter(double xn) override;
};
