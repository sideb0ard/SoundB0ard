#include <math.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter.h"
#include "filter_csem.h"

// typedef struct filter_csem {
//
//    FILTER *bc_filter; // base class
//    double m_alpha;
//    double m_alpha0;
//    double m_rho;
//    double m_z11;
//    double m_z12;
//
//} FILTER_CSEM;

FILTER_CSEM *new_filter_csem()
{
    FILTER_CSEM *filter = (FILTER_CSEM *)calloc(1, sizeof(FILTER_CSEM));

    filter->bc_filter = new_filter();
    filter->bc_filter->m_aux_control = 0.5;
    filter->bc_filter->m_type = LPF2;

    filter->m_alpha = 1.0;
    filter->m_alpha0 = 1.0;
    filter->m_rho = 1.0;

    csem_reset(filter);
    return filter;
}

void csem_reset(void *filter)
{
    FILTER_CSEM *self = filter;
    self->m_z11 = 0;
    self->m_z12 = 0;
}

void csem_set_qcontrol(void *filter, double qcontrol)
{
    FILTER_CSEM *self = filter;
    self->bc_filter->m_q_control =
        (25.0 - 0.5) * (qcontrol - 1.0) / (10.0 - 1.0) + 0.5;
}

void csem_update(void *filter)
{
    FILTER_CSEM *self = filter;
    filter_update(self->bc_filter); // update base class

    double wd = 2 * M_PI * self->bc_filter->m_fc;
    double T = 1.0 / SAMPLE_RATE;
    double wa = (2 / T) * tan(wd * T / 2);
    double g = wa * T / 2;
    double R = 1.0 / (2.0 * self->bc_filter->m_q);
    self->m_alpha0 = 1.0 / (1.0 + 2.0 * R * g + g * g);
    self->m_alpha = g;
    self->m_rho = 2.0 * R + g;
}

double csem_gennext(void *filter, double xn)
{
    FILTER_CSEM *self = filter;
    if (self->bc_filter->m_type != LPF2 && self->bc_filter->m_type != HPF2 &&
        self->bc_filter->m_type != BPF2 && self->bc_filter->m_type != BSF2)
        return xn;

    double hpf =
        self->m_alpha0 * (xn - self->m_rho * self->m_z11 - self->m_z12);
    double bpf = self->m_alpha * hpf + self->m_z11;

    if (self->bc_filter->m_nlp == ON)
        bpf = tanh(self->bc_filter->m_saturation * bpf);

    double lpf = self->m_alpha * bpf + self->m_z12;
    // double R = 1.0 / ( 2.0 * self->bc_filter->m_q);

    // double bsf = xn - 2.0 * R * bpf; // hmm, this is unused - mistake?

    double semBSF = self->bc_filter->m_aux_control * hpf +
                    (1.0 - self->bc_filter->m_aux_control) * lpf;

    self->m_z11 = self->m_alpha * hpf + bpf;
    self->m_z12 = self->m_alpha * bpf + lpf;
    if (self->bc_filter->m_type == LPF2)
        return lpf;
    else if (self->bc_filter->m_type == HPF2)
        return hpf;
    else if (self->bc_filter->m_type == BPF2)
        return bpf;
    else if (self->bc_filter->m_type == BSF2)
        return semBSF;

    // shouldn't get here
    return xn;
}
