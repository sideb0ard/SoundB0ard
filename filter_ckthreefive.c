#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter.h"
#include "filter_ckthreefive.h"
#include "filter_onepole.h"

filter_ck35 *new_filter_ck35()
{
    filter_ck35 *ck35 = (filter_ck35 *)calloc(1, sizeof(filter_ck35));
    filter_setup(&ck35->f);

    onepole_setup(&ck35->m_LPF1);
    ck35->m_LPF1.f.m_filter_type = LPF1;

    onepole_setup(&ck35->m_LPF2);
    ck35->m_LPF2.f.m_filter_type = LPF2;

    onepole_setup(&ck35->m_HPF1);
    ck35->m_HPF1.f.m_filter_type = HPF1;

    onepole_setup(&ck35->m_HPF2);
    ck35->m_HPF2.f.m_filter_type = HPF2;

    ck35->f.m_filter_type = LPF2; // default

    ck35->m_k = 0.1;

    ck35->f.set_fc_mod = &filter_set_fc_mod;
    ck35->f.set_q_control = &ck_set_qcontrol;
    ck35->f.gennext = &ck_gennext;
    ck35->f.update = &ck_update;
    ck35->f.reset = &filter_reset;

    return ck35;
}

void ck_set_qcontrol(filter *f, double qcontrol)
{
    filter_ck35 *self = (filter_ck35 *)f;
    self->m_k = (2.0 - 0.01) * (qcontrol - 1.0) / (10.0 - 1.0) + 0.01;
}

void ck_update(filter *f)
{
    filter_update(f); // update base class
    filter_ck35 *ck35 = (filter_ck35 *)f;

    double wd = 2.0 * M_PI * f->m_fc;
    double T = 1.0 / SAMPLE_RATE;
    double wa = (2.0 / T) * tan(wd * T / 2.0);
    double g = wa * T / 2.0;

    double G = g / (1.0 + g);

    ck35->m_LPF1.m_alpha = G;
    ck35->m_LPF2.m_alpha = G;
    ck35->m_HPF1.m_alpha = G;
    ck35->m_HPF2.m_alpha = G;

    ck35->m_alpha0 = 1.0 / (1.0 - ck35->m_k * G + ck35->m_k * G * G);

    if (f->m_filter_type == LPF2) {
        ck35->m_LPF2.m_beta = (ck35->m_k - ck35->m_k * G) / (1.0 + g);
        ck35->m_LPF1.m_beta = -1.0 / (1.0 + g);
        // printf("UPDATE LPF2 mmmbeta %f\n", self->m_LPF2->m_beta);
        // printf("UPDATE LPF1 mmmbeta %f\n", self->m_LPF1->m_beta);
    }
    else // HPF
    {
        ck35->m_HPF2.m_beta = -1.0 * G / (1.0 + g);
        ck35->m_HPF1.m_beta = 1.0 / (1.0 + g);
    }
}

void ck_reset(filter *f)
{
    filter_ck35 *ck35 = (filter_ck35 *)f;
    onepole_reset((filter *)&ck35->m_LPF1); // ew, hacky abstraction leak
    onepole_reset((filter *)&ck35->m_LPF2); // ew, hacky abstraction leak
    onepole_reset((filter *)&ck35->m_HPF1); // ew, hacky abstraction leak
    onepole_reset((filter *)&ck35->m_HPF2); // ew, hacky abstraction leak
}

double ck_gennext(filter *f, double xn)
{
    if (f->m_filter_type != LPF2 && f->m_filter_type != HPF2)
        return xn;

    filter_ck35 *ck35 = (filter_ck35 *)f;

    double y = 0.0;
    if (f->m_filter_type == LPF2) {
        double y1 = onepole_gennext((filter *)&ck35->m_LPF1, xn);
        // printf("Y1! %f\n", y1);

        double S35 = onepole_get_feedback_output(&ck35->m_HPF1) +
                     onepole_get_feedback_output(&ck35->m_LPF2);
        // printf("S35! %f\n", S35);

        double u = ck35->m_alpha0 * (y1 + S35);
        // printf("U! %f\n", u);

        if (f->m_nlp == ON)
            u = tanh(f->m_saturation * u);

        y = ck35->m_k * onepole_gennext((filter *)&ck35->m_LPF2, u);
        // printf("Y! %f\n", y);

        onepole_gennext((filter *)&ck35->m_HPF1, y);
    }
    else // HPF
    {
        double y1 = onepole_gennext((filter *)&ck35->m_HPF1, xn);

        double S35 = onepole_get_feedback_output(&ck35->m_HPF2) +
                     onepole_get_feedback_output(&ck35->m_LPF1);
        double u = ck35->m_alpha0 * y1 + S35;
        y = ck35->m_k * u;

        if (f->m_nlp == ON)
            y = tanh(f->m_saturation * y);

        onepole_gennext((filter *)&ck35->m_LPF1,
                        onepole_gennext((filter *)&ck35->m_HPF2, y));
    }
    if (ck35->m_k > 0)
        y *= 1.0 / ck35->m_k;

    // printf("CK Y2K beeeatch! - Y: %f\n", y);
    return y;
}
