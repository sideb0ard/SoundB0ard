#pragma once

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"

struct MoogLadder : public Filter
{

    MoogLadder();
    ~MoogLadder() = default;

    double m_k;
    double m_gamma;
    double m_alpha_0;

    double m_a;
    double m_b;
    double m_c;
    double m_d;
    double m_e;

    OnePole m_LPF1;
    OnePole m_LPF2;
    OnePole m_LPF3;
    OnePole m_LPF4;

    void SetQControl(double qcontrol) override;
    void Reset() override;
    void Update() override;
    double DoFilter(double xn) override;
};
