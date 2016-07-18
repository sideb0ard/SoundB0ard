#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter_onepole.h"

static inline void print_vals(FILTER_ONEPOLE *self)
{
    printf("BASECLASS VALS\n");
    printf("NLP: %d\n", self->bc_filter->m_nlp);
    printf("Freq Cutoff: %f\n", self->bc_filter->m_fc);
    printf("Q: %f\n", self->bc_filter->m_q);
    printf("FC Mod: %f\n", self->bc_filter->m_fc_mod);
    printf("SELF\n");
    printf("Alpha: %f\n", self->m_alpha);
    printf("Beta: %f\n", self->m_beta);
    printf("Z1: %f\n", self->m_z1);
    printf("Gamma: %f\n", self->m_gamma);
    printf("Delta: %f\n", self->m_delta);
    printf("Epsilon: %f\n", self->m_epsilon);
    printf("a0: %f\n", self->m_a0);
    printf("Feedback: %f\n", self->m_feedback);
}

FILTER_ONEPOLE *new_filter_onepole(void)
{
    FILTER_ONEPOLE *filter =
        (FILTER_ONEPOLE *)calloc(1, sizeof(FILTER_ONEPOLE));

    filter->bc_filter = new_filter();

    filter->bc_filter->gennext = &onepole_gennext;
    filter->bc_filter->update = &onepole_update;
    filter->bc_filter->reset = &onepole_reset;

    filter->bc_filter->m_type = LPF1;

    filter->m_alpha = 1.0;
    filter->m_beta = 0.0;
    filter->m_gamma = 1.0;
    filter->m_delta = 0.0;
    filter->m_epsilon = 0.0;
    filter->m_feedback = 0.0;
    filter->m_a0 = 1.0;
    filter->m_z1 = 0.0;

    onepole_reset(filter); // seems redundant as they were set, but following the book here
    return filter;
}

void onepole_update(void *filter)
{
    FILTER_ONEPOLE *self = filter;

    filter_update(self->bc_filter); // update base class

    double wd = 2 * M_PI * self->bc_filter->m_fc;
    double T = 1.0 / SAMPLE_RATE;
    double wa = (2 / T) * tan(wd * T / 2);
    double g = wa * T / 2;

    self->m_alpha = g / (1.0 + g);
}

void onepole_set_feedback(void *filter, double fb)
{
    FILTER_ONEPOLE *self = filter;
    self->m_feedback = fb;
}

double onepole_get_feedback_output(void *filter)
{
    FILTER_ONEPOLE *self = filter;
    return self->m_beta * (self->m_z1 + self->m_feedback * self->m_delta);
}

void onepole_reset(void *filter)
{
    FILTER_ONEPOLE *self = filter;
    self->m_z1 = 0.0;
    self->m_feedback = 0.0;
}

double onepole_gennext(void *filter, double xn)
{
    FILTER_ONEPOLE *self = filter;
    if (self->bc_filter->m_type != LPF1 && self->bc_filter->m_type != HPF1)
        return xn;

    xn = xn * self->m_gamma + self->m_feedback +
         self->m_epsilon * onepole_get_feedback_output(self);

    double vn = (self->m_a0 * xn - self->m_z1) * self->m_alpha;

    double lpf = vn + self->m_z1;

    if (lpf > 1.0) {
        print_vals(self);
    }

    self->m_z1 = vn + lpf;

    double hpf = xn - lpf;

    if (self->bc_filter->m_type == LPF1)
        return lpf;
    else if (self->bc_filter->m_type == HPF1)
        return hpf;

    return xn; // should never get here
}
