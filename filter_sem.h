#pragma once

#include "defjams.h"
#include "filter.h"

typedef struct filter_sem filter_sem;

struct filter_sem
{

    filter f; // base class
    double m_alpha;
    double m_alpha0;
    double m_rho;
    double m_z11;
    double m_z12;
};

filter_sem *new_filter_sem(void);

void sem_set_qcontrol(filter *f, double qcontrol);
void sem_reset(filter *f);
void sem_update(filter *f);
double sem_gennext(filter *f, double xn);
