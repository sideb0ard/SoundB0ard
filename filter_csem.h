#pragma once

#include "defjams.h"
#include "filter.h"

typedef struct filter_csem {

    FILTER *bc_filter; // base class
    double m_alpha;
    double m_alpha0;
    double m_rho;
    double m_z11;
    double m_z12;

} FILTER_CSEM;

FILTER_CSEM *new_filter_csem(void);
void csem_reset(void *filter);
void csem_update(void *filter);
double csem_gennext(void *filter, double xn);
void csem_set_qcontrol(void *filter, double qcontrol);
