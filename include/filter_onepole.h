#pragma once

#include "defjams.h"
#include "filter.h"

struct OnePole : public Filter
{

    OnePole();
    ~OnePole() = default;

    double m_alpha;
    double m_beta;
    double m_z1;
    double m_gamma;
    double m_delta;
    double m_epsilon;
    double m_a0;
    double m_feedback;

    double DoFilter(double xn);
    void Update();
    void SetFeedback(double fb);
    double GetFeedbackOutput();
    void Reset();
    void SetFilterType(filter_type ftype); // ENUM in filter.h
};
