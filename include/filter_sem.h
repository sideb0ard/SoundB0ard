#pragma once

#include "defjams.h"
#include "filter.h"

struct FilterSem : public Filter
{

    FilterSem();
    ~FilterSem() = default;

    double m_alpha;
    double m_alpha0;
    double m_rho;
    double m_z11;
    double m_z12;

    void SetQControl(double qcontrol);
    void Reset();
    void Update();
    double DoFilter(double xn);
};
