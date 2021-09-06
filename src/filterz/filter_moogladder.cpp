#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <defjams.h>
#include <filter.h>
#include <filter_moogladder.h>
#include <filter_onepole.h>
#include <utils.h>

MoogLadder::MoogLadder()
{
    m_LPF1.m_filter_type = LPF1;
    m_LPF2.m_filter_type = LPF1;
    m_LPF3.m_filter_type = LPF1;

    m_LPF4.m_filter_type = LPF1;

    m_filter_type = LPF4; // default

    m_k = 0.0;
    m_alpha_0 = 1.0;
    m_a = 0.0;
    m_b = 0.0;
    m_c = 0.0;
    m_d = 0.0;
    m_e = 0.0;

    m_q_control = 1.0; // Q is 1 to 10
}

void MoogLadder::SetQControl(double qcontrol)
{
    if (qcontrol >= 1 && qcontrol <= 10)
    {
        m_q_control = qcontrol;
        m_k = (4.0) * (qcontrol - 1.0) / (10.0 - 1.0);
    }
}

void MoogLadder::Update()
{
    Filter::Update();
    m_k = (4.0) * (m_q_control - 1.0) / (10.0 - 1.0);

    double wd = 2.0 * M_PI * m_fc;
    static double T = 1.0 / SAMPLE_RATE;
    double wa = (2.0 / T) * tan(wd * T / 2.0);
    double g = wa * T / 2.0;

    double G = g / (1.0 + g);

    m_LPF1.m_alpha = G;
    m_LPF2.m_alpha = G;
    m_LPF3.m_alpha = G;
    m_LPF4.m_alpha = G;

    m_LPF1.m_beta = G * G * G / (1.0 + g);
    m_LPF2.m_beta = G * G / (1.0 + g);
    m_LPF3.m_beta = G / (1.0 + g);
    m_LPF4.m_beta = 1.0 / (1.0 + g);

    m_gamma = G * G * G * G;
    m_alpha_0 = 1.0 / (1.0 + m_k * m_gamma);

    switch (m_filter_type)
    {
    case LPF4:
        m_a = 0.0;
        m_b = 0.0;
        m_c = 0.0;
        m_d = 0.0;
        m_e = 1.0;
        break;
    case LPF2:
        m_a = 0.0;
        m_b = 0.0;
        m_c = 1.0;
        m_d = 0.0;
        m_e = 0.0;
        break;
    case BPF4:
        m_a = 0.0;
        m_b = 0.0;
        m_c = 4.0;
        m_d = -8.0;
        m_e = 4.0;
        break;
    case BPF2:
        m_a = 0.0;
        m_b = 2.0;
        m_c = -2.0;
        m_d = 0.0;
        m_e = 0.0;
        break;
    case HPF4:
        m_a = 1.0;
        m_b = -4.0;
        m_c = 6.0;
        m_d = -4.0;
        m_e = -1.0;
        break;
    case HPF2:
        m_a = 1.0;
        m_b = -2.0;
        m_c = 1.0;
        m_d = 0.0;
        m_e = 0.0;
        break;
    default: // LPF4
        m_a = 0.0;
        m_b = 0.0;
        m_c = 0.0;
        m_d = 0.0;
        m_e = 1.0;
    }
}

void MoogLadder::Reset()
{
    m_LPF1.Reset();
    m_LPF2.Reset();
    m_LPF3.Reset();
    m_LPF4.Reset();
}

double MoogLadder::DoFilter(double xn)
{
    if (m_filter_type == BSF2 || m_filter_type == LPF1 || m_filter_type == HPF1)
        return xn;

    double sigma = m_LPF1.GetFeedbackOutput() + m_LPF2.GetFeedbackOutput() +
                   m_LPF3.GetFeedbackOutput() + m_LPF4.GetFeedbackOutput();

    xn *= 1.0 + m_aux_control * m_k;

    double u = (xn - m_k * sigma) * m_alpha_0;
    if (m_nlp)
    {
        u = fasttanh(m_saturation * u);
    }

    double LP1 = m_LPF1.DoFilter(u);
    double LP2 = m_LPF1.DoFilter(u);
    double LP3 = m_LPF1.DoFilter(u);
    double LP4 = m_LPF1.DoFilter(u);

    return m_a * u + m_b * LP1 + m_c * LP2 + m_d * LP3 + m_e * LP4;
}
