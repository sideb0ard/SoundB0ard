#pragma once

#include <fx/fx.h>

// LoFiCrusher: Unified bit depth and sample rate reduction
// Combines the best features of BitCrush and Decimate
class LoFiCrusher : public Fx {
 public:
  LoFiCrusher();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void Update();

  // Bit depth reduction (quantization)
  void SetBitdepth(int val);
  float Quantize(float sample, float step);

  // Sample rate reduction (sample and hold)
  void SetSampleHoldFreq(double val);

  // Destructive quantization (from Decimate)
  void SetDestruct(float val);
  float DestructQuantize(float sample, float destruct);

  // Parameters
  int bitdepth_{8};               // Bit depth: 1-16 bits
  double sample_hold_freq_{1.0};  // Sample hold: 0.0-1.0 (1.0 = no reduction)
  float destruct_{0.0};           // Destructive amount: 0.0-1.0

  // State
  double phasor_{0.0};
  float last_left_{0.0};
  float last_right_{0.0};
  float step_{0.0};
  float inv_step_{1.0};
  int samples_left_{0};
};
