#include <stdlib.h>
#include <math.h>

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"
#include "filter_ckthreefive.h"

FILTER_CK35 *new_filter_ck35(void)
{
    FILTER_CK35 *filter = (FILTER_CK35 *) calloc(1, sizeof(FILTER_CK35));
    filter->bc_filter = new_filter();

    filter->m_LPF1 = new_filter_onepole();
    onepole_set_filter_type(filter->m_LPF1, LPF1);

    filter->m_LPF2 = new_filter_onepole();
    onepole_set_filter_type(filter->m_LPF2, LPF2);

    filter->m_HPF1 = new_filter_onepole();
    onepole_set_filter_type(filter->m_HPF1, HPF1);

    filter->m_HPF2 = new_filter_onepole();
    onepole_set_filter_type(filter->m_HPF2, HPF2);

    filter->m_filter_type = LPF2; // default

    return filter;
}

void ck_set_qcontrol(void *filter, double qcontrol)
{
    FILTER_CK35 *self = filter;
    self->m_k = (2.0 - 0.01)*(self->bc_filter->m_q_control - 1.0)/(10.0 - 1.0) + 0.01;
}

void ck_update(void *filter)
{
    FILTER_CK35 *self = filter;
    filter_update(self->bc_filter); // update base class

    double wd = 2.0 * M_PI * self->bc_filter->m_fc;
    double T = 1.0 / SAMPLE_RATE;
    double wa = ( 2.0 / T) * tan(wd*T/2.0);
    double g = wa * T / 2.0;

    double G = g / (1.0 + g);

    self->m_LPF1->m_alpha = G;
    self->m_LPF2->m_alpha = G;
    self->m_HPF1->m_alpha = G;
    self->m_HPF2->m_alpha = G;

    self->m_alpha0 = 1.0 / ( 1.0 - self->m_k*G + self->m_k*G*G );

    if ( self->m_filter_type == LPF2 )
    {
        self->m_LPF2->m_beta = ( self->m_k - self->m_k * G ) / (1.0 + g);
        self->m_LPF1->m_beta = -1.0/(1.0 + g);
    }
    else // HPF
    {
        self->m_HPF2->m_beta = -1.0*G/(1.0 + g);
        self->m_HPF1->m_beta = 1.0/(1.0 + g);
    }
}

void ck_reset(void *filter) {
    FILTER_CK35 *self = (FILTER_CK35 *) filter;
    onepole_reset(self->m_LPF1);
    onepole_reset(self->m_LPF2);
    onepole_reset(self->m_HPF1);
    onepole_reset(self->m_HPF2);
}

double ck_gennext(void *filter, double xn)
{
    FILTER_CK35 *self = filter;
    if (self->m_filter_type != LPF2 && self->m_filter_type != HPF2)
        return xn;

    double y = 0.0;
    if ( self->m_filter_type == LPF2)
    {
        double y1 = onepole_gennext(self->m_LPF1, xn);

        double S35 = onepole_get_feedback_output(self->m_HPF1) +
                     onepole_get_feedback_output(self->m_LPF2);

        double u = self->m_alpha0 *  (y1 + S35);

        if (self->bc_filter->m_nlp == ON)
            u = tanh(self->bc_filter->m_saturation * u);

        y = self->m_k * onepole_gennext(self->m_LPF2, u);

        onepole_gennext(self->m_HPF1, y);
    }
    else // HPF
    {
        double y1 = onepole_gennext(self->m_HPF1, xn);

        double S35 = onepole_get_feedback_output(self->m_HPF2) +
                     onepole_get_feedback_output(self->m_LPF1);
        double u = self->m_alpha0 * y1 + S35;
        y = self->m_k * u;

        if (self->bc_filter->m_nlp == ON)
            y = tanh(self->bc_filter->m_saturation * y);

        onepole_gennext(self->m_LPF1, onepole_gennext(self->m_HPF2, y));
    }
    if (self->m_k > 0)
        y *= 1.0 / self->m_k;

    return y;
}
