#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter.h"
#include "filter_moogladder.h"
#include "filter_onepole.h"
#include "utils.h"

filter_moog *new_filter_moog()
{
    filter_moog *moog = (filter_moog *)calloc(1, sizeof(filter_moog));
    filter_moog_init(moog);

    return moog;
}

void filter_moog_init(filter_moog *moog)
{
    filter_setup(&moog->f);

    onepole_setup(&moog->m_LPF1);
    moog->m_LPF1.f.m_filter_type = LPF1;

    onepole_setup(&moog->m_LPF2);
    moog->m_LPF2.f.m_filter_type = LPF1;

    onepole_setup(&moog->m_LPF3);
    moog->m_LPF3.f.m_filter_type = LPF1;

    onepole_setup(&moog->m_LPF4);
    moog->m_LPF4.f.m_filter_type = LPF1;

    moog->f.m_filter_type = LPF4; // default

    moog->m_k = 0.0;
    moog->m_alpha_0 = 1.0;
    moog->m_a = 0.0;
    moog->m_b = 0.0;
    moog->m_c = 0.0;
    moog->m_d = 0.0;
    moog->m_e = 0.0;

    moog->f.set_fc_mod = &filter_set_fc_mod;
    moog->f.set_q_control = &moog_set_qcontrol;
    moog->f.gennext = &moog_gennext;
    moog->f.update = &moog_update;
    moog->f.m_q_control = 1.0; // Q is 1 to 10
    moog->f.reset = &filter_reset;
}

void moog_set_qcontrol(filter *f, double qcontrol)
{
    filter_moog *self = (filter_moog *)f;
    self->f.m_q_control = qcontrol;
    self->m_k = (4.0) * (qcontrol - 1.0) / (10.0 - 1.0);
    // printf("M_K: %f\n", self->m_k);
}

void moog_update(filter *f)
{
    filter_update(f); // update base class
    filter_moog *moog = (filter_moog *)f;
    moog->m_k = (4.0) * (f->m_q_control - 1.0) / (10.0 - 1.0);

    double wd = 2.0 * M_PI * f->m_fc;
    static double T = 1.0 / SAMPLE_RATE;
    double wa = (2.0 / T) * tan(wd * T / 2.0);
    double g = wa * T / 2.0;

    double G = g / (1.0 + g);

    moog->m_LPF1.m_alpha = G;
    moog->m_LPF2.m_alpha = G;
    moog->m_LPF3.m_alpha = G;
    moog->m_LPF4.m_alpha = G;

    moog->m_LPF1.m_beta = G * G * G / (1.0 + g);
    moog->m_LPF2.m_beta = G * G / (1.0 + g);
    moog->m_LPF3.m_beta = G / (1.0 + g);
    moog->m_LPF4.m_beta = 1.0 / (1.0 + g);

    moog->m_gamma = G * G * G * G;
    // moog->m_alpha_0 = 1.0 / (1.0 + moog->m_k * G + moog->m_k * G * G);
    moog->m_alpha_0 = 1.0 / (1.0 + moog->m_k * moog->m_gamma);

    switch (moog->f.m_filter_type)
    {
    case LPF4:
        moog->m_a = 0.0;
        moog->m_b = 0.0;
        moog->m_c = 0.0;
        moog->m_d = 0.0;
        moog->m_e = 1.0;
        break;
    case LPF2:
        moog->m_a = 0.0;
        moog->m_b = 0.0;
        moog->m_c = 1.0;
        moog->m_d = 0.0;
        moog->m_e = 0.0;
        break;
    case BPF4:
        moog->m_a = 0.0;
        moog->m_b = 0.0;
        moog->m_c = 4.0;
        moog->m_d = -8.0;
        moog->m_e = 4.0;
        break;
    case BPF2:
        moog->m_a = 0.0;
        moog->m_b = 2.0;
        moog->m_c = -2.0;
        moog->m_d = 0.0;
        moog->m_e = 0.0;
        break;
    case HPF4:
        moog->m_a = 1.0;
        moog->m_b = -4.0;
        moog->m_c = 6.0;
        moog->m_d = -4.0;
        moog->m_e = -1.0;
        break;
    case HPF2:
        moog->m_a = 1.0;
        moog->m_b = -2.0;
        moog->m_c = 1.0;
        moog->m_d = 0.0;
        moog->m_e = 0.0;
        break;
    default: // LPF4
        moog->m_a = 0.0;
        moog->m_b = 0.0;
        moog->m_c = 0.0;
        moog->m_d = 0.0;
        moog->m_e = 1.0;
    }
}

void moog_reset(filter *f)
{
    filter_moog *moog = (filter_moog *)f;
    onepole_reset((filter *)&moog->m_LPF1);
    onepole_reset((filter *)&moog->m_LPF2);
    onepole_reset((filter *)&moog->m_LPF3);
    onepole_reset((filter *)&moog->m_LPF4);
}

double moog_gennext(filter *f, double xn)
{
    if (f->m_filter_type == BSF2 || f->m_filter_type == LPF1 ||
        f->m_filter_type == HPF1)
        return xn;

    filter_moog *moog = (filter_moog *)f;
    double sigma = onepole_get_feedback_output(&moog->m_LPF1) +
                   onepole_get_feedback_output(&moog->m_LPF2) +
                   onepole_get_feedback_output(&moog->m_LPF3) +
                   onepole_get_feedback_output(&moog->m_LPF4);

    xn *= 1.0 + f->m_aux_control * moog->m_k;

    double u = (xn - moog->m_k * sigma) * moog->m_alpha_0;
    if (f->m_nlp)
    {
        u = fasttanh(f->m_saturation * u);
    }

    double LP1 = onepole_gennext((filter *)&moog->m_LPF1, u);
    double LP2 = onepole_gennext((filter *)&moog->m_LPF2, LP1);
    double LP3 = onepole_gennext((filter *)&moog->m_LPF3, LP2);
    double LP4 = onepole_gennext((filter *)&moog->m_LPF4, LP3);

    return moog->m_a * u + moog->m_b * LP1 + moog->m_c * LP2 + moog->m_d * LP3 +
           moog->m_e * LP4;
}
