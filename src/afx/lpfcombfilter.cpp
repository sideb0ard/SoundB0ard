#include <afx/lpfcombfilter.h>
#include <defjams.h>

void lpf_comb_filter_set_comb_g(lpf_comb_filter *l, double comb_g) {
  l->m_comb_g = comb_g;
}

void lpf_comb_filter_set_lpf_g(lpf_comb_filter *l, double over_all_gain) {
  l->m_lpf_g = over_all_gain * (1.0 - l->m_comb_g);
}

void lpf_comb_filter_set_comb_g_with_rt_sixty(lpf_comb_filter *l, double rt) {
  double exponent = -3.0 * l->m_delay.m_delay_in_samples * (1.0 / SAMPLE_RATE);
  rt /= 1000.0;

  l->m_comb_g = pow((float)10.0, exponent / rt);
}

void lpf_comb_filter_init(lpf_comb_filter *l, int delay_length) {
  l->m_comb_g = 0;
  l->m_lpf_g = 0;
  l->m_lpf_z1 = 0;
  delay_init(&l->m_delay, delay_length);
}

bool lpf_comb_filter_process_audio(lpf_comb_filter *l, double *in,
                                   double *out) {
  double yn = delay_read_delay(&l->m_delay);
  if (l->m_delay.m_read_index == l->m_delay.m_write_index) yn = 0;

  double yn_lpf = yn + l->m_lpf_g * l->m_lpf_z1;

  l->m_lpf_z1 = yn_lpf;

  double fb = *in + l->m_comb_g * yn_lpf;

  delay_write_delay_and_inc(&l->m_delay, fb);

  if (l->m_delay.m_read_index == l->m_delay.m_write_index) yn = *in;

  *out = yn;

  return true;
}
