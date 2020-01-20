#pragma once

#include "defjams.h"
#include "filter.h"
#include "filter_onepole.h"

typedef struct filter_moogladder filter_moog;

typedef struct filter_moogladder
{

    filter f; // base class
    double m_k;
    double m_gamma;
    double m_alpha_0;

    double m_a;
    double m_b;
    double m_c;
    double m_d;
    double m_e;

    filter_onepole m_LPF1;
    filter_onepole m_LPF2;
    filter_onepole m_LPF3;
    filter_onepole m_LPF4;
} filter_moogladder;

filter_moog *new_filter_moog(void);
void filter_moog_init(filter_moog *f);

void moog_set_qcontrol(filter *f, double qcontrol);
void moog_reset(filter *f);
void moog_update(filter *f);
double moog_gennext(filter *f, double xn);
