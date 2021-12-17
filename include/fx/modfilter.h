#pragma once

#include "biquad.h"
#include "fx.h"
#include "wt_oscillator.h"

class ModFilter : Fx {
 public:
  ModFilter();
  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;
  double GetParam(std::string name) override;

 private:
  void Init();
  void Update();
  double CalculateCutoffFreq(double lfo_sample);
  double CalculateQ(double lfo_sample);
  void CalculateLeftLpfCoeffs(double cutoff_freq, double q);
  void CalculateRightLpfCoeffs(double cutoff_freq, double q);
  void SetModDepthFc(double val);
  void SetModRateFc(double val);
  void SetModDepthQ(double val);
  void SetModRateQ(double val);
  void SetLfoWaveForm(unsigned int val);
  void SetLfoPhase(unsigned int val);

 private:
  biquad m_left_lpf_;
  biquad m_right_lpf_;

  WTOscillator m_fc_lfo_;
  WTOscillator m_q_lfo_;

  double m_min_cutoff_freq_;
  double m_max_cutoff_freq_;
  double m_min_q_;
  double m_max_q_;

  double m_mod_depth_fc_;
  double m_mod_rate_fc_;
  double m_mod_depth_q_;
  double m_mod_rate_q_;
  unsigned int m_lfo_waveform_;
  unsigned int m_lfo_phase_;
};
