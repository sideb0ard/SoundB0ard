#pragma once
#include <stdbool.h>

#include "delay.h"

typedef struct comb_filter {
  delay m_delay;    // base class
  double m_comb_g;  // one co-efficient
} comb_filter;

void comb_filter_init(comb_filter *c, int delay_len);
void comb_filter_set_comb_g(comb_filter *c, double comb_g);
void comb_filter_set_comb_g_with_rt_sixty(comb_filter *c, double rt);
bool comb_filter_process_audio(comb_filter *c, double *in, double *out);
