#pragma once

#include <defjams.h>
#include <fx/fx.h>

class BitCrush : Fx {
 public:
  BitCrush();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void SetBitdepth(int val);
  void SetBitrate(int val);
  void SetSampleHoldFreq(double val);
  void Update();

 private:
  int bitdepth_;
  int bitrate_;
  double sample_hold_freq_;
  double step_;
  double inv_step_;
  double phasor_;
  double last_left_;
  double last_right_;
};
