#include <fx/onepolelpf.h>
#include <stdbool.h>

void one_pole_lpf_set_lpf_g(one_pole_lpf *o, double g) {
  o->m_lpf_g = g;
}

void one_pole_lpf_init(one_pole_lpf *o) {
  o->m_lpf_g = 0;
  o->m_lpf_z1 = 0;
}

bool one_pole_lpf_process_audio(one_pole_lpf *o, double *in, double *out) {
  double yn_lpf = *in * (1.0 - o->m_lpf_g) + o->m_lpf_g * o->m_lpf_z1;
  o->m_lpf_z1 = yn_lpf;
  *out = yn_lpf;
  return true;
}
