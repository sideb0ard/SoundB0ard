#pragma once

#include <defjams.h>
#include <filter_moogladder.h>
#include <fx/fx.h>
#include <lfo.h>
#include <stdbool.h>

enum { LOWPASS, HIGHPASS, ALLPASS, BANDPASS };

class FilterPass : public Fx {
 public:
  FilterPass();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void SetLfoActive(int lfo_num, bool b);
  void SetLfoRate(int lfo_num, double val);
  void SetLfoAmp(int lfo_num, double val);
  void SetLfoType(int lfo_num, unsigned int type);
  void Update();

 private:
  std::unique_ptr<MoogLadder> m_filter_;

  LFO m_lfo1_;  // route to freq
  bool m_lfo1_active_{false};

  LFO m_lfo2_;  // route to qv
  bool m_lfo2_active_{false};
};
