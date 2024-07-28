#pragma once
#include <stdbool.h>

typedef struct one_pole_lpf {
  double m_lpf_g;
  double m_lpf_z1;
} one_pole_lpf;

void one_pole_lpf_set_lpf_g(one_pole_lpf *o, double g);
void one_pole_lpf_init(one_pole_lpf *o);
bool one_pole_lpf_process_audio(one_pole_lpf *o, double *in, double *out);
