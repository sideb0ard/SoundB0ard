#pragma once

#include "defjams.h"
#include "filter.h"

typedef struct filter_onepole {

    filter f; // base class
    double m_alpha;
    double m_beta;
    double m_z1;
    double m_gamma;
    double m_delta;
    double m_epsilon;
    double m_a0;
    double m_feedback;

} filter_onepole;

filter_onepole *new_filter_onepole(void);
void onepole_setup(filter_onepole *op);

double onepole_gennext(filter *f, double xn);
void onepole_update(filter *f);
void onepole_set_feedback(filter_onepole *f, double fb);
double onepole_get_feedback_output(filter_onepole *f);
void onepole_reset(filter *f);
void onepole_set_filter_type(filter *f, filter_type ftype); // ENUM in filter.h
