#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter.h"
#include "filter_ckthreefive.h"
#include "filter_onepole.h"

CKThreeFive::CKThreeFive()
{
    m_LPF1.m_filter_type = LPF1;
    m_LPF2.m_filter_type = LPF2;

    m_HPF1.m_filter_type = HPF1;
    m_HPF2.m_filter_type = HPF2;

    m_filter_type = LPF2; // default

    m_k = 0.1;
    m_alpha0 = 0;

    Reset();
}

void CKThreeFive::SetQControl(double qcontrol)
{
    m_k = (2.0 - 0.01) * (qcontrol - 1.0) / (10.0 - 1.0) + 0.01;
}

void CKThreeFive::Update()
{
    Filter::Update();

    double wd = 2.0 * M_PI * m_fc;
    double T = 1.0 / SAMPLE_RATE;
    double wa = (2.0 / T) * tan(wd * T / 2.0);
    double g = wa * T / 2.0;

    double G = g / (1.0 + g);

    m_LPF1.m_alpha = G;
    m_LPF2.m_alpha = G;
    m_HPF1.m_alpha = G;
    m_HPF2.m_alpha = G;

    m_alpha0 = 1.0 / (1.0 - m_k * G + m_k * G * G);

    if (m_filter_type == LPF2)
    {
        m_LPF2.m_beta = (m_k - m_k * G) / (1.0 + g);
        m_HPF1.m_beta = -1.0 / (1.0 + g);
    }
    else // HPF
    {
        m_HPF2.m_beta = -1.0 * G / (1.0 + g);
        m_LPF1.m_beta = 1.0 / (1.0 + g);
    }
}

void CKThreeFive::Reset()
{
    m_LPF1.Reset();
    m_LPF2.Reset();
    m_HPF1.Reset();
    m_HPF2.Reset();
}

double CKThreeFive::DoFilter(double xn)
{
    if (m_filter_type != LPF2 && m_filter_type != HPF2)
        return xn;

    double y = 0.0;
    if (m_filter_type == LPF2)
    {
        double y1 = m_LPF1.DoFilter(xn);

        double S35 = m_HPF1.GetFeedbackOutput() + m_LPF2.GetFeedbackOutput();

        double u = m_alpha0 * (y1 + S35);

        if (m_nlp)
            u = tanh(m_saturation * u);

        y = m_k * m_LPF2.DoFilter(u);

        m_HPF1.DoFilter(y);
    }
    else // HPF
    {
        double y1 = m_HPF1.DoFilter(xn);

        double S35 = m_HPF2.GetFeedbackOutput() + m_LPF1.GetFeedbackOutput();
        double u = m_alpha0 * y1 + S35;
        y = m_k * u;

        if (m_nlp)
            y = tanh(m_saturation * y);

        m_LPF1.DoFilter(m_HPF2.DoFilter(y));
    }
    if (m_k > 0)
        y *= 1.0 / m_k;

    return y;
}
