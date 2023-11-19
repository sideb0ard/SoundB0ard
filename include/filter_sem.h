#pragma once

#include "defjams.h"
#include "filter.h"

struct FilterSem : public Filter {
  FilterSem();
  ~FilterSem() = default;

  double m_alpha{1.0};
  double m_alpha0{1.0};
  double m_rho{1.0};
  double m_z11{0.0};
  double m_z12{0.0};

  void SetQControl(double qcontrol) override;
  void Reset() override;
  void Update() override;
  double DoFilter(double xn) override;
};
