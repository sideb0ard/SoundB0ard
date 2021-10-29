#include <afx/combfilter.h>
#include <defjams.h>

void comb_filter_init(comb_filter *c, int delay_len) {
  delay_init(&c->m_delay, delay_len);
  c->m_comb_g = 0.0;
}

void comb_filter_set_comb_g(comb_filter *c, double comb_g) {
  c->m_comb_g = comb_g;
}

void comb_filter_set_comb_g_with_rt_sixty(comb_filter *c, double rt) {
  double exponent = -3.0 * c->m_delay.m_delay_in_samples * (1.0 / SAMPLE_RATE);
  rt /= 1000.0;
  c->m_comb_g = pow((float)10.0, exponent / rt);
}

bool comb_filter_process_audio(comb_filter *c, double *in, double *out) {
  double yn = delay_read_delay(&c->m_delay);

  if (c->m_delay.m_read_index == c->m_delay.m_write_index) yn = 0;

  double fb = *in + c->m_comb_g * yn;

  delay_write_delay_and_inc(&c->m_delay, fb);

  if (c->m_delay.m_read_index == c->m_delay.m_write_index) yn = *in;

  *out = yn;

  return true;
}
