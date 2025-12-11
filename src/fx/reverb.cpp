#include <defjams.h>
#include <fx/reverb.h>
#include <stdio.h>
#include <stdlib.h>

#include <sstream>

Reverb::Reverb() {
  m_pre_delay_msec = 40;
  m_pre_delay_atten_db = 0;
  m_input_lpf_g = 0.45;

  m_apf_1_delay_msec = 13.28;
  m_apf_1_g = 0.7;

  m_apf_2_delay_msec = 28.13;
  m_apf_2_g = -0.7;

  m_comb_1_delay_msec = 31.71;
  m_comb_2_delay_msec = 37.11;
  m_comb_3_delay_msec = 40.23;
  m_comb_4_delay_msec = 44.14;
  m_comb_5_delay_msec = 30.47;
  m_comb_6_delay_msec = 33.98;
  m_comb_7_delay_msec = 41.41;
  m_comb_8_delay_msec = 42.58;

  m_apf_3_delay_msec = 9.38;
  m_apf_3_g = -0.6;

  m_apf_4_delay_msec = 11;
  m_apf_4_g = 0.6;

  m_lpf2_g2 = 0.25;
  m_rt60 = 100;
  m_wet_pct = 20;

  Init();

  type_ = fx_type::REVERB;
  enabled_ = true;
}

StereoVal Reverb::Process(StereoVal in) {
  double input_sample = in.left + in.right * 0.5;

  double pre_delay_out = 0;
  delay_process_audio(&m_pre_delay, &input_sample, &pre_delay_out);

  double apf_1_out = 0;
  delay_apf_process_audio(&m_input_apf_1, &pre_delay_out, &apf_1_out);

  double apf_2_out = 0;
  delay_apf_process_audio(&m_input_apf_2, &apf_1_out, &apf_2_out);

  double input_lpf = 0;
  one_pole_lpf_process_audio(&m_input_lpf, &apf_2_out, &input_lpf);

  double pc_1_out = 0;
  double pc_2_out = 0;
  double pc_3_out = 0;
  double pc_4_out = 0;
  double pc_5_out = 0;
  double pc_6_out = 0;
  double pc_7_out = 0;
  double pc_8_out = 0;
  double c_1_out = 0;
  double c_2_out = 0;

  comb_filter_process_audio(&m_parallel_cf_1, &input_lpf, &pc_1_out);
  comb_filter_process_audio(&m_parallel_cf_2, &input_lpf, &pc_2_out);
  lpf_comb_filter_process_audio(&m_parallel_cf_3, &input_lpf, &pc_3_out);
  lpf_comb_filter_process_audio(&m_parallel_cf_4, &input_lpf, &pc_4_out);
  comb_filter_process_audio(&m_parallel_cf_5, &input_lpf, &pc_5_out);
  comb_filter_process_audio(&m_parallel_cf_6, &input_lpf, &pc_6_out);
  lpf_comb_filter_process_audio(&m_parallel_cf_7, &input_lpf, &pc_7_out);
  lpf_comb_filter_process_audio(&m_parallel_cf_8, &input_lpf, &pc_8_out);

  c_1_out =
      0.25 * pc_1_out - 0.25 * pc_2_out + 0.25 * pc_3_out - 0.25 * pc_4_out;
  c_2_out =
      0.25 * pc_5_out - 0.25 * pc_6_out + 0.25 * pc_7_out - 0.25 * pc_8_out;

  double damping_lpf_1_out = 0;
  one_pole_lpf_process_audio(&m_damping_lpf_1, &c_1_out, &damping_lpf_1_out);

  double damping_lpf_2_out = 0;
  one_pole_lpf_process_audio(&m_damping_lpf_2, &c_2_out, &damping_lpf_2_out);

  double apf_3_out = 0;
  delay_apf_process_audio(&m_output_apf_3, &damping_lpf_1_out, &apf_3_out);

  double apf_4_out = 0;
  delay_apf_process_audio(&m_output_apf_4, &damping_lpf_2_out, &apf_4_out);

  StereoVal out = {};
  out.left = ((100.0 - m_wet_pct) / 100.0) * input_sample +
             (m_wet_pct / 100.0) * (apf_3_out);

  out.right = ((100.0 - m_wet_pct) / 100.0) * input_sample +
              (m_wet_pct / 100.0) * (apf_4_out);

  return out;
}

std::string Reverb::Status() {
  std::stringstream ss;
  ss << "Reverb! predelayms:" << m_pre_delay_msec;
  ss << " reverbtime:" << m_rt60;
  ss << " wetmx:" << m_wet_pct;

  return ss.str();
}

void Reverb::SetParam(std::string name, double val) {
  if (name == "predelayms")
    SetPreDelayMsec(val);
  else if (name == "reverbtime")
    SetRt60(val);
  else if (name == "wetmx")
    SetWetPct(val);
  CookVariables();
}

void Reverb::Init() {
  delay_init(&m_pre_delay, 2.0 * SAMPLE_RATE);

  delay_apf_init(&m_input_apf_1, 0.1 * SAMPLE_RATE);
  delay_apf_init(&m_input_apf_2, 0.1 * SAMPLE_RATE);

  comb_filter_init(&m_parallel_cf_1, 0.1 * SAMPLE_RATE);
  comb_filter_init(&m_parallel_cf_2, 0.1 * SAMPLE_RATE);
  lpf_comb_filter_init(&m_parallel_cf_3, 0.1 * SAMPLE_RATE);
  lpf_comb_filter_init(&m_parallel_cf_4, 0.1 * SAMPLE_RATE);
  comb_filter_init(&m_parallel_cf_5, 0.1 * SAMPLE_RATE);
  comb_filter_init(&m_parallel_cf_6, 0.1 * SAMPLE_RATE);
  lpf_comb_filter_init(&m_parallel_cf_7, 0.1 * SAMPLE_RATE);
  lpf_comb_filter_init(&m_parallel_cf_8, 0.1 * SAMPLE_RATE);

  delay_apf_init(&m_output_apf_3, 0.1 * SAMPLE_RATE);
  delay_apf_init(&m_output_apf_4, 0.1 * SAMPLE_RATE);

  one_pole_lpf_init(&m_input_lpf);
  one_pole_lpf_init(&m_damping_lpf_1);
  one_pole_lpf_init(&m_damping_lpf_2);

  delay_reset_delay(&m_pre_delay);
  delay_reset_delay(&m_input_apf_1.m_delay);
  delay_reset_delay(&m_input_apf_2.m_delay);

  delay_reset_delay(&m_parallel_cf_1.m_delay);
  delay_reset_delay(&m_parallel_cf_2.m_delay);
  delay_reset_delay(&m_parallel_cf_3.m_delay);
  delay_reset_delay(&m_parallel_cf_4.m_delay);
  delay_reset_delay(&m_parallel_cf_5.m_delay);
  delay_reset_delay(&m_parallel_cf_6.m_delay);
  delay_reset_delay(&m_parallel_cf_7.m_delay);
  delay_reset_delay(&m_parallel_cf_8.m_delay);

  delay_reset_delay(&m_output_apf_3.m_delay);
  delay_reset_delay(&m_output_apf_4.m_delay);

  CookVariables();
}

void Reverb::CookVariables() {
  // pre-delay
  delay_set_delay_ms(&m_pre_delay, m_pre_delay_msec);
  delay_set_output_attenuation_db(&m_pre_delay, m_pre_delay_atten_db);

  // input diffusion
  delay_set_delay_ms(&m_input_apf_1.m_delay, m_apf_1_delay_msec);
  delay_apf_set_apf_g(&m_input_apf_1, m_apf_1_g);

  delay_set_delay_ms(&m_input_apf_2.m_delay, m_apf_2_delay_msec);
  delay_apf_set_apf_g(&m_input_apf_2, m_apf_2_g);

  // output diffusion
  delay_set_delay_ms(&m_output_apf_3.m_delay, m_apf_3_delay_msec);
  delay_apf_set_apf_g(&m_output_apf_3, m_apf_3_g);

  delay_set_delay_ms(&m_output_apf_4.m_delay, m_apf_4_delay_msec);
  delay_apf_set_apf_g(&m_output_apf_4, m_apf_4_g);

  delay_set_delay_ms(&m_parallel_cf_1.m_delay, m_comb_1_delay_msec);
  delay_set_delay_ms(&m_parallel_cf_2.m_delay, m_comb_2_delay_msec);
  delay_set_delay_ms(&m_parallel_cf_3.m_delay, m_comb_3_delay_msec);
  delay_set_delay_ms(&m_parallel_cf_4.m_delay, m_comb_4_delay_msec);
  delay_set_delay_ms(&m_parallel_cf_5.m_delay, m_comb_5_delay_msec);
  delay_set_delay_ms(&m_parallel_cf_6.m_delay, m_comb_6_delay_msec);
  delay_set_delay_ms(&m_parallel_cf_7.m_delay, m_comb_7_delay_msec);
  delay_set_delay_ms(&m_parallel_cf_8.m_delay, m_comb_8_delay_msec);

  comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_1, m_rt60);
  comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_2, m_rt60);
  lpf_comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_3, m_rt60);
  lpf_comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_4, m_rt60);
  comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_5, m_rt60);
  comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_6, m_rt60);
  lpf_comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_7, m_rt60);
  lpf_comb_filter_set_comb_g_with_rt_sixty(&m_parallel_cf_8, m_rt60);

  // LPFszz
  one_pole_lpf_set_lpf_g(&m_damping_lpf_1, m_lpf2_g2);
  one_pole_lpf_set_lpf_g(&m_damping_lpf_2, m_lpf2_g2);
  one_pole_lpf_set_lpf_g(&m_input_lpf, m_input_lpf_g);

  // LPF-comb filters
  lpf_comb_filter_set_comb_g(&m_parallel_cf_3, m_lpf2_g2);
  lpf_comb_filter_set_comb_g(&m_parallel_cf_4, m_lpf2_g2);
  lpf_comb_filter_set_comb_g(&m_parallel_cf_7, m_lpf2_g2);
  lpf_comb_filter_set_comb_g(&m_parallel_cf_8, m_lpf2_g2);
}

void Reverb::SetPreDelayMsec(double val) {
  if (val >= 0 && val <= 100)
    m_pre_delay_msec = val;
  else
    printf("Val must be between 0 and 100\n");
}

void Reverb::SetPreDelayAttenDb(double val) {
  if (val >= -96 && val <= 0)
    m_pre_delay_atten_db = val;
  else
    printf("Val must be between -96 and 0\n");
}

void Reverb::SetRt60(double val) {
  if (val >= 0 && val <= 5000)
    m_rt60 = val;
  else
    printf("Val must be between 0 and 5000\n");
}

void Reverb::SetWetPct(double val) {
  if (val >= 0 && val <= 100)
    m_wet_pct = val;
  else
    printf("Val must be between 0 and 100\n");
}

void Reverb::SetInputLpfG(double val) {
  if (val >= 0 && val <= 1)
    m_input_lpf_g = val;
  else
    printf("Val must be between 0 and 1\n");
}

void Reverb::SetLpf2G2(double val) {
  if (val >= 0 && val <= 1)
    m_lpf2_g2 = val;
  else
    printf("Val must be between 0 and 1\n");
}

void Reverb::SetApfDelayMsec(int apf_num, double val) {
  if (val >= 0 && val <= 100 && apf_num > 0 && apf_num < 5) {
    switch (apf_num) {
      case (1):
        m_apf_1_delay_msec = val;
      case (2):
        m_apf_2_delay_msec = val;
      case (3):
        m_apf_3_delay_msec = val;
      case (4):
        m_apf_4_delay_msec = val;
    }
  } else
    printf("Val must be between 0 and 100, apf 1-4\n");
}

void Reverb::SetApfG(int apf_num, double val) {
  if (val >= -1 && val <= 1 && apf_num > 0 && apf_num < 5) {
    switch (apf_num) {
      case (1):
        m_apf_1_g = val;
      case (2):
        m_apf_2_g = val;
      case (3):
        m_apf_3_g = val;
      case (4):
        m_apf_4_g = val;
    }
  } else
    printf("Val must be between -1 and 1, apf 1-4\n");
}

void Reverb::SetCombDelayMsec(int comb_num, double val) {
  if (val >= 0 && val <= 100 && comb_num > 0 && comb_num < 9) {
    switch (comb_num) {
      case (1):
        m_comb_1_delay_msec = val;
      case (2):
        m_comb_2_delay_msec = val;
      case (3):
        m_comb_3_delay_msec = val;
      case (4):
        m_comb_4_delay_msec = val;
      case (5):
        m_comb_5_delay_msec = val;
      case (6):
        m_comb_6_delay_msec = val;
      case (7):
        m_comb_7_delay_msec = val;
      case (8):
        m_comb_8_delay_msec = val;
    }
  } else
    printf("Val must be between 0 and 100, Combs 1-8\n");
}
