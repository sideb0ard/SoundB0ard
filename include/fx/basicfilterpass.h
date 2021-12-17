#pragma once

#include <defjams.h>
#include <filter_moogladder.h>
#include <fx/fx.h>
#include <lfo.h>
#include <stdbool.h>

enum { LOWPASS, HIGHPASS, ALLPASS, BANDPASS };

class FilterPass : Fx {
 public:
  FilterPass();
  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;
  double GetParam(std::string name) override;

 private:
  void SetLfoActive(int lfo_num, bool b);
  void SetLfoRate(int lfo_num, double val);
  void SetLfoAmp(int lfo_num, double val);
  void SetLfoType(int lfo_num, unsigned int type);

 private:
  MoogLadder m_filter_;

  LFO m_lfo1_;  // route to freq
  bool m_lfo1_active_{true};

  LFO m_lfo2_;  // route to qv
  bool m_lfo2_active_{false};
};
