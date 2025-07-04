#include <defjams.h>
#include <fx/ddlmodule.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

DDLModule::DDLModule() {
  ResetDelay();
}

void DDLModule::Update() {
  m_feedback = m_feedback_pct / 100.0;
  m_wet_level = m_wet_level_pct / 100.0;
  m_delay_in_samples = m_delay_ms * (SAMPLE_RATE / 1000.0);

  m_read_index = m_write_index - (int)m_delay_in_samples;

  if (m_read_index < 0) m_read_index += m_buffer.size();
}

void DDLModule::ResetDelay() {
  m_buffer.fill(0);
  m_write_index = 0;
  m_read_index = 0;
}

void DDLModule::SetDelayMs(double delay_time_ms) {
  m_delay_ms = delay_time_ms;
  Update();
}

double DDLModule::ReadDelay() {
  double yn = m_buffer.at(m_read_index);
  int read_index_1 = m_read_index - 1;
  if (read_index_1 < 0) {
    read_index_1 = m_buffer.size() - 1;
  }
  double yn_1 = m_buffer.at(read_index_1);

  double frac_delay = m_delay_in_samples - (int)m_delay_in_samples;

  return utils::LinTerp(0, 1, yn, yn_1, frac_delay);
}

double DDLModule::ReadDelayAt(double time_in_ms) {
  double delay_in_samples = time_in_ms * (SAMPLE_RATE / 1000.);
  int read_index = m_write_index - (int)delay_in_samples;

  if (read_index < 0) read_index += m_buffer.size();

  double yn = m_buffer.at(read_index);

  int read_index_1 = read_index - 1;
  if (read_index_1 < 0) read_index_1 = m_buffer.size() - 1;

  double yn_1 = m_buffer.at(read_index_1);

  double frac_delay = m_delay_in_samples - (int)m_delay_in_samples;

  return utils::LinTerp(0, 1, yn, yn_1, frac_delay);
}

void DDLModule::WriteDelayAndInc(double delay_input) {
  m_buffer[m_write_index] = delay_input;

  m_write_index++;
  if (m_write_index >= (int)m_buffer.size()) m_write_index = 0;

  m_read_index++;
  if (m_read_index >= (int)m_buffer.size()) m_read_index = 0;
}

bool DDLModule::ProcessAudio(double *input, double *output) {
  *output = m_delay_in_samples == 0 ? *input : ReadDelay();
  WriteDelayAndInc(*input);
  return true;
}
