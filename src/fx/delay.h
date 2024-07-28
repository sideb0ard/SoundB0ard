#pragma once

#include <math.h>
#include <stdbool.h>

struct delay {
  double m_delay_in_samples;
  double m_output_attenuation;
  std::vector<double> m_buffer;

  int m_read_index;
  int m_write_index;
  int m_buffer_size;

  double m_delay_ms;
  double m_output_attenuation_db;
};

void delay_init(delay *d, int delay_len);
void delay_cook_variables(delay *d);
void delay_reset_delay(delay *d);
void delay_set_delay_ms(delay *d, double ms);
void delay_set_output_attenuation_db(delay *d, double adb);
double delay_read_delay(delay *d);
double delay_read_delay_at(delay *d, double ms);
void delay_write_delay_and_inc(delay *d, double delay_input);
bool delay_process_audio(delay *d, double *in, double *out);
