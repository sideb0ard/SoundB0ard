#include <math.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter.h"
#include "filter_sem.h"

filter_sem *new_filter_sem()
{
    filter_sem *cs = (filter_sem *)calloc(1, sizeof(filter_sem));

    filter_setup(&cs->f);

    cs->f.m_aux_control = 0.5;
    cs->f.m_filter_type = LPF2;

    cs->m_alpha = 1.0;
    cs->m_alpha0 = 1.0;
    cs->m_rho = 1.0;

    sem_reset(&cs->f);

    cs->f.set_fc_mod = &filter_set_fc_mod;
    cs->f.gennext = &sem_gennext;
    cs->f.update = &sem_update;
    cs->f.reset = &sem_update;
    cs->f.set_q_control = &sem_set_qcontrol;

    return cs;
}

void sem_reset(filter *f)
{
    filter_sem *self = (filter_sem *)f;
    self->m_z11 = 0;
    self->m_z12 = 0;
}

void sem_set_qcontrol(filter *f, double qcontrol)
{
    f->m_q = (25.0 - 0.5) * (qcontrol - 1.0) / (10.0 - 1.0) + 0.5;
}

void sem_update(filter *f)
{
    filter_update(f); // update base class

    filter_sem *cs = (filter_sem *)f;

    double wd = 2 * M_PI * f->m_fc;
    double T = 1.0 / SAMPLE_RATE;
    double wa = (2 / T) * tan(wd * T / 2);
    double g = wa * T / 2;
    double R = 1.0 / (2.0 * f->m_q);
    cs->m_alpha0 = 1.0 / (1.0 + 2.0 * R * g + g * g);
    cs->m_alpha = g;
    cs->m_rho = 2.0 * R + g;
}

double sem_gennext(filter *f, double xn)
{
    if (f->m_filter_type != LPF2 && f->m_filter_type != HPF2 && f->m_filter_type != BPF2 &&
        f->m_filter_type != BSF2)
        return xn;

    filter_sem *cs = (filter_sem *)f;

    double hpf = cs->m_alpha0 * (xn - cs->m_rho * cs->m_z11 - cs->m_z12);

    double bpf = cs->m_alpha * hpf + cs->m_z11;

    if (f->m_nlp == ON)
        bpf = tanh(f->m_saturation * bpf);

    double lpf = cs->m_alpha * bpf + cs->m_z12;

    // double R = 1.0 / (2.0 * f->m_q);

    // double bsf = xn - 2.0 * R * bpf; // hmm, this is unused - mistake?

    double semBSF = f->m_aux_control * hpf + (1.0 - f->m_aux_control) * lpf;

    cs->m_z11 = cs->m_alpha * hpf + bpf;
    cs->m_z12 = cs->m_alpha * bpf + lpf;

    if (f->m_filter_type == LPF2)
        return lpf;
    else if (f->m_filter_type == HPF2)
        return hpf;
    else if (f->m_filter_type == BPF2)
        return bpf;
    else if (f->m_filter_type == BSF2)
        return semBSF;

    // shouldn't get here
    return xn;
}
