#pragma once

#include "defjams.h"
#include "filter.h"

typedef struct filter_onepole
{
    FILTER bc_filter; // base class

    double m_alpha;
    double m_beta;
    double m_z1;
    double m_gamma;
    double m_delta;
    double m_epsilon;
    double m_da0;
    double m_feedback;


} FILTER_ONEPOLE;

FILTER_ONEPOLE *new_filter_onepole(void);
double onepole_gennext(void *filter, double xn);
void onepole_update(void *filter);
void onepole_set_feedback(void *filter, double fb);
double onepole_get_feedback_output(void *filter);
void onepole_reset(void *filter);
