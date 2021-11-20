#pragma once
#include "delay.h"
#include <stdbool.h>

typedef struct lpf_comb_filter
{
    delay m_delay;
    double m_comb_g; // comb coefficient
    double m_lpf_g;  // lpf coefficient
    double m_lpf_z1; // one sample delay
} lpf_comb_filter;

void lpf_comb_filter_set_comb_g(lpf_comb_filter *l, double g);
void lpf_comb_filter_set_comb_g_with_rt_sixty(lpf_comb_filter *l, double rt);
void lpf_comb_filter_set_lpf_g(lpf_comb_filter *l, double g);
void lpf_comb_filter_init(lpf_comb_filter *l, int delay_len);
bool lpf_comb_filter_process_audio(lpf_comb_filter *l, double *in, double *out);
