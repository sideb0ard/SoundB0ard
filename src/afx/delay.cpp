#include <afx/delay.h>
#include <defjams.h>
#include <stdlib.h>
#include <strings.h>
#include <utils.h>

void delay_init(delay *d, int delay_length) {
  d->m_buffer = NULL;
  d->m_output_attenuation_db = 0;
  d->m_delay_ms = 0.0;
  d->m_output_attenuation = 0.0;
  d->m_delay_in_samples = 0.0;

  d->m_write_index = 0;
  d->m_read_index = 0;

  d->m_buffer_size = delay_length;
  d->m_buffer = (double *)calloc(d->m_buffer_size, sizeof(double));
}

void delay_reset_delay(delay *d) {
  if (d->m_buffer) memset(d->m_buffer, 0, d->m_buffer_size * sizeof(double));
  d->m_write_index = 0;
  d->m_read_index = 0;
  delay_cook_variables(d);
}

void delay_cook_variables(delay *d) {
  d->m_output_attenuation =
      pow((float)10.0, (float)d->m_output_attenuation_db / (float)20.0);
  d->m_delay_in_samples = d->m_delay_ms * (SAMPLE_RATE / 1000.0);
  d->m_read_index = d->m_write_index - (int)d->m_delay_in_samples;
  if (d->m_read_index < 0) d->m_read_index += d->m_buffer_size;
}

void delay_set_delay_ms(delay *d, double ms) {
  d->m_delay_ms = ms;
  delay_cook_variables(d);
}

void delay_set_output_attenuation_db(delay *d, double atten) {
  d->m_output_attenuation_db = atten;
  delay_cook_variables(d);
}

void delay_write_delay_and_inc(delay *d, double in) {
  d->m_buffer[d->m_write_index] = in;

  d->m_write_index++;
  if (d->m_write_index >= d->m_buffer_size) d->m_write_index = 0;

  d->m_read_index++;
  if (d->m_read_index >= d->m_buffer_size) d->m_read_index = 0;
}

double delay_read_delay(delay *d) {
  double yn = d->m_buffer[d->m_read_index];
  int read_index_1 = d->m_read_index - 1;
  if (read_index_1 < 0) read_index_1 = d->m_buffer_size - 1;

  double yn_1 = d->m_buffer[read_index_1];

  double frac_delay = d->m_delay_in_samples - (int)d->m_delay_in_samples;

  return utils::LinTerp(0, 1, yn, yn_1, frac_delay);
}

double delay_read_delay_at(delay *d, double msec) {
  double delay_in_samples = msec * ((double)SAMPLE_RATE / 1000.0);
  int read_index = d->m_write_index - (int)delay_in_samples;

  double yn = d->m_buffer[read_index];

  int read_index_1 = read_index - 1;
  if (read_index_1 < 0) read_index_1 = d->m_buffer_size - 1;

  double yn_1 = d->m_buffer[read_index_1];

  double frac_delay = d->m_delay_in_samples - (int)d->m_delay_in_samples;

  return utils::LinTerp(0, 1, yn, yn_1, frac_delay);
}

bool delay_process_audio(delay *d, double *in, double *out) {
  double xn = *in;
  double yn = d->m_delay_in_samples == 0 ? xn : delay_read_delay(d);
  delay_write_delay_and_inc(d, xn);
  *out = d->m_output_attenuation * yn;
  return true;
}
