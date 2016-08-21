#pragma once

#include "defjams.h"
#include "filter.h"

typedef struct filter_ckthreefive {

    FILTER *bc_filter; // base class
    double m_k;
    double m_alpha0;

    filter_type m_filter_type;

    FILTER_ONEPOLE *m_LPF1;
    FILTER_ONEPOLE *m_LPF2;
    FILTER_ONEPOLE *m_HPF1;
    FILTER_ONEPOLE *m_HPF2;

} FILTER_CK35;

FILTER_CK35 *new_filter_ck35(void);
void ck_update(void *filter);
void ck_set_qcontrol(void *filter, double qcontrol);
void ck_reset(void *filter);
double ck_gennext(void *filter, double xn);
