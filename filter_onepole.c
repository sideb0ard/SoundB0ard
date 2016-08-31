#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter_onepole.h"

// static inline void print_vals(FILTER_ONEPOLE *self)
//{
//    printf("BASECLASS VALS\n");
//    printf("NLP: %d\n", self->bc_filter->m_nlp);
//    printf("Freq Cutoff: %f\n", self->bc_filter->m_fc);
//    printf("Q: %f\n", self->bc_filter->m_q);
//    printf("FC Mod: %f\n", self->bc_filter->m_fc_mod);
//    printf("SELF\n");
//    printf("Alpha: %f\n", self->m_alpha);
//    printf("Beta: %f\n", self->m_beta);
//    printf("Z1: %f\n", self->m_z1);
//    printf("Gamma: %f\n", self->m_gamma);
//    printf("Delta: %f\n", self->m_delta);
//    printf("Epsilon: %f\n", self->m_epsilon);
//    printf("a0: %f\n", self->m_a0);
//    printf("Feedback: %f\n", self->m_feedback);
//}

filter_onepole *new_filter_onepole()
{
    filter_onepole *op =
        (filter_onepole *)calloc(1, sizeof(filter_onepole));

    filter_setup(&op->f);
    onepole_setup(op);

    return op;
}

void onepole_setup(filter_onepole *op)
{
    op->f.set_fc_mod = &filter_set_fc_mod;
    op->f.gennext = &onepole_gennext;
    op->f.update = &onepole_update;
    op->f.reset = &onepole_reset;

    op->f.m_type = LPF1;

    op->m_alpha = 1.0;
    op->m_beta = 0.0;
    op->m_gamma = 1.0;
    op->m_delta = 0.0;
    op->m_epsilon = 0.0;
    op->m_feedback = 0.0;
    op->m_a0 = 1.0;
    op->m_z1 = 0.0;
}

void onepole_update(filter *f)
{
    filter_update(f); // update base class

    filter_onepole *op = (filter_onepole *) f;

    double wd = 2.0 * M_PI * f->m_fc;
    double T = 1.0 / SAMPLE_RATE;
    double wa = (2.0 / T) * tan(wd * T / 2.0);
    double g = wa * T / 2.0;

    op->m_alpha = g / (1.0 + g);
}

void onepole_set_feedback(filter_onepole *op, double fb)
{
    op->m_feedback = fb;
}

double onepole_get_feedback_output(filter_onepole *f)
{
    return f->m_beta * (f->m_z1 + f->m_feedback * f->m_delta);
}

void onepole_reset(filter *self)
{
    filter_onepole *f = (filter_onepole *) self;
    f->m_z1 = 0.0;
    f->m_feedback = 0.0;
}

double onepole_gennext(filter *f, double xn)
{
    if (f->m_type != LPF1 && f->m_type != HPF1)
        return xn;

    filter_onepole *op = (filter_onepole *) f;

    xn = xn * op->m_gamma + op->m_feedback +
         op->m_epsilon * onepole_get_feedback_output(op);

    double vn = (op->m_a0 * xn - op->m_z1) * op->m_alpha;

    double lpf = vn + op->m_z1;

    op->m_z1 = vn + lpf;

    double hpf = xn - lpf;

    if (f->m_type == LPF1)
        return lpf;
    else if (f->m_type == HPF1)
        return hpf;

    return xn; // should never get here
}

