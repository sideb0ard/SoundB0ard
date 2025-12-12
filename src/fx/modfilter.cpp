#include <defjams.h>
#include <fx/modfilter.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <sstream>

ModFilter::ModFilter() {
  Init();
  type_ = fx_type::MODFILTER;
  enabled_ = true;
}

void ModFilter::Init() {
  m_left_lpf_.FlushDelays();
  m_right_lpf_.FlushDelays();

  m_min_cutoff_freq_ = 100.;
  m_max_cutoff_freq_ = 5000.;
  m_min_q_ = 0.577;
  m_max_q_ = 10;

  m_lfo_waveform_ = sine;
  m_mod_depth_fc_ = 50.;
  m_mod_rate_fc_ = 2.5;
  m_mod_depth_q_ = 50.;
  m_mod_rate_q_ = 2.5;
  m_wet_mix_ = 100.;  // 100% wet by default
  m_lfo_phase_ = 0;

  // m_fc_lfo.polarity = 1; // unipolar
  // m_fc_lfo.mode = 0;     // normal, not band limited

  // m_q_lfo.polarity = 1; // unipolar
  // m_q_lfo.mode = 0;     // normal, not band limited

  Update();

  m_fc_lfo_.StartOscillator();
  m_q_lfo_.StartOscillator();
}

void ModFilter::Update() {
  m_fc_lfo_.m_osc_fo = m_mod_rate_fc_;
  m_q_lfo_.m_osc_fo = m_mod_rate_q_;
  m_fc_lfo_.m_waveform = m_lfo_waveform_;
  m_q_lfo_.m_waveform = m_lfo_waveform_;
  m_fc_lfo_.Update();
  m_q_lfo_.Update();
}

double ModFilter::CalculateCutoffFreq(double lfo_sample) {
  // Convert bipolar LFO (-1 to 1) to unipolar (0 to 1)
  double lfo_unipolar = (lfo_sample + 1.0) / 2.0;

  double fc = (m_mod_depth_fc_ / 100.0) *
                  (lfo_unipolar * (m_max_cutoff_freq_ - m_min_cutoff_freq_)) +
              m_min_cutoff_freq_;

  // Clamp to safe range
  if (fc < 20.0) fc = 20.0;
  if (fc > 20000.0) fc = 20000.0;

  return fc;
}

double ModFilter::CalculateQ(double lfo_sample) {
  // Convert bipolar LFO (-1 to 1) to unipolar (0 to 1)
  double lfo_unipolar = (lfo_sample + 1.0) / 2.0;

  double q = (m_mod_depth_q_ / 100.0) * (lfo_unipolar * (m_max_q_ - m_min_q_)) +
             m_min_q_;

  // Clamp to safe range to prevent filter instability
  if (q < 0.5) q = 0.5;
  if (q > 20.0) q = 20.0;

  return q;
}

void ModFilter::CalculateLeftLpfCoeffs(double cutoff_freq, double q) {
  double theta_c = 2.0 * M_PI * cutoff_freq / (double)SAMPLE_RATE;
  double d = 1.0 / q;

  double beta_numerator = 1.0 - ((d / 2.0) * (sin(theta_c)));
  double beta_denominator = 1.0 + ((d / 2.0) * (sin(theta_c)));

  double beta = 0.5 * (beta_numerator / beta_denominator);

  double gamma = (0.5 + beta) * (cos(theta_c));

  double alpha = (0.5 + beta - gamma) / 2.0;

  // left channel
  m_left_lpf_.m_a0 = alpha;
  m_left_lpf_.m_a1 = 2.0 * alpha;
  m_left_lpf_.m_a2 = alpha;
  m_left_lpf_.m_b1 =
      -2.0 * gamma;  // if b's are negative in the difference equation
  m_left_lpf_.m_b2 = 2.0 * beta;
}

void ModFilter::CalculateRightLpfCoeffs(double cutoff_freq, double q) {
  double theta_c = 2.0 * M_PI * cutoff_freq / (double)SAMPLE_RATE;
  double d = 1.0 / q;

  double beta_numerator = 1.0 - ((d / 2.0) * (sin(theta_c)));
  double beta_denominator = 1.0 + ((d / 2.0) * (sin(theta_c)));

  double beta = 0.5 * (beta_numerator / beta_denominator);

  double gamma = (0.5 + beta) * (cos(theta_c));

  double alpha = (0.5 + beta - gamma) / 2.0;

  // right channel
  m_right_lpf_.m_a0 = alpha;
  m_right_lpf_.m_a1 = 2.0 * alpha;
  m_right_lpf_.m_a2 = alpha;
  m_right_lpf_.m_b1 =
      -2.0 * gamma;  // if b's are negative in the difference equation
  m_right_lpf_.m_b2 = 2.0 * beta;
}

StereoVal ModFilter::Process(StereoVal in) {
  StereoVal out = {};

  double yn = 0.0;
  double yqn = 0;  // quad phase

  yn = m_fc_lfo_.DoOscillate(&yqn);
  double fc = CalculateCutoffFreq(yn);
  double fcq = CalculateCutoffFreq(yqn);
  // printf("LFO FC: %f FC: %f\n", yn, fc);

  yn = m_q_lfo_.DoOscillate(&yqn);
  double q = CalculateQ(yn);
  double qq = CalculateQ(yqn);
  // printf("LFO Q: %f Q: %f\n", yn, q);

  CalculateLeftLpfCoeffs(fc, q);

  if (m_lfo_phase_ == 0)
    CalculateRightLpfCoeffs(fc, q);
  else
    CalculateRightLpfCoeffs(fcq, qq);

  // Process through filters (wet signal)
  double wet_left = m_left_lpf_.Process(in.left);
  double wet_right = m_right_lpf_.Process(in.right);

  // Mix dry and wet signals
  // wet_mix_ = 0% -> all dry, 100% -> all wet
  double wet_amount = m_wet_mix_ / 100.0;
  double dry_amount = 1.0 - wet_amount;

  out.left = (in.left * dry_amount) + (wet_left * wet_amount);
  out.right = (in.right * dry_amount) + (wet_right * wet_amount);

  return out;
}

std::string ModFilter::Status() {
  std::stringstream ss;
  ss << "depthfc:" << m_mod_depth_fc_;
  ss << " ratefc:" << m_mod_rate_fc_;
  ss << " depthq:" << m_mod_depth_q_;
  ss << " rateq:" << m_mod_rate_q_;
  ss << " mix:" << m_wet_mix_;
  ss << " lfo:" << m_lfo_waveform_;
  ss << " phase:" << m_lfo_phase_;

  return ss.str();
}

void ModFilter::SetModDepthFc(double val) {
  if (val >= 0 && val <= 100)
    m_mod_depth_fc_ = val;
  else
    printf("Val has to be between 0 and 100\n");

  Update();
}

void ModFilter::SetModRateFc(double val) {
  if (val >= 0.02 && val <= 10)
    m_mod_rate_fc_ = val;
  else
    printf("Val has to be between 0.2 and 10\n");

  Update();
}

void ModFilter::SetModDepthQ(double val) {
  if (val >= 0 && val <= 100)
    m_mod_depth_q_ = val;
  else
    printf("Val has to be between 0 and 100\n");

  Update();
}

void ModFilter::SetModRateQ(double val) {
  if (val >= 0.02 && val <= 10)
    m_mod_rate_q_ = val;
  else
    printf("Val has to be between 0.2 and 10\n");

  Update();
}

void ModFilter::SetLfoWaveForm(unsigned int val) {
  if (val < 4)
    m_lfo_waveform_ = val;
  else
    printf("Val has to be between 0 and 3\n");

  Update();
}

void ModFilter::SetLfoPhase(unsigned int val) {
  if (val < 2)
    m_lfo_phase_ = val;
  else
    printf("Val has to be between 0 or 1\n");

  Update();
}

void ModFilter::SetWetMix(double val) {
  if (val >= 0 && val <= 100)
    m_wet_mix_ = val;
  else
    printf("Val has to be between 0 and 100\n");
}

void ModFilter::SetParam(std::string name, double val) {
  if (name == "depthfc") {
    SetModDepthFc(val);
  } else if (name == "ratefc") {
    SetModRateFc(val);
  } else if (name == "depthq") {
    SetModDepthQ(val);
  } else if (name == "rateq") {
    SetModRateQ(val);
  } else if (name == "mix") {
    SetWetMix(val);
  } else if (name == "lfo") {
    SetLfoWaveForm((unsigned int)val);
  } else if (name == "phase") {
    SetLfoPhase((unsigned int)val);
  } else {
    printf("Unknown parameter '%s' for ModFilter\n", name.c_str());
  }
}
