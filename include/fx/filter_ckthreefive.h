#pragma once

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"

typedef struct filter_ckthreefive filter_ckthreefive;
typedef struct filter_ckthreefive filter_ck35;

struct filter_ckthreefive
{

    filter f; // base class
    double m_k;
    double m_alpha0;

    filter_type m_filter_type;

    filter_onepole m_LPF1;
    filter_onepole m_LPF2;
    filter_onepole m_HPF1;
    filter_onepole m_HPF2;
};

filter_ck35 *new_filter_ck35(void);
void filter_ck35_init(filter_ck35 *ck35);

void ck_update(filter *f);
void ck_set_qcontrol(filter *f, double qcontrol);
void ck_reset(filter *f);
double ck_gennext(filter *f, double xn);
