#pragma once

#include <defjams.h>
#include <fx/fx.h>

// Distortion modes
enum class DistortionMode {
  HARD_CLIP = 0,  // Hard clipping (brick wall limiter)
  SOFT_CLIP = 1,  // Soft clipping (tanh)
  TUBE = 2,       // Tube-style asymmetric saturation
  FOLDBACK = 3    // Foldback/wavefold distortion
};

class Distortion : public Fx {
 public:
  Distortion();

  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void SetThreshold(double val);
  void SetDrive(double val);
  void SetMode(int val);

  // Processing functions for each mode
  double ProcessHardClip(double input);
  double ProcessSoftClip(double input);
  double ProcessTube(double input);
  double ProcessFoldback(double input);

  DistortionMode mode_{DistortionMode::HARD_CLIP};
  double threshold_{0.5};  // Clipping threshold (0.01 - 1.0)
  double drive_{1.0};      // Input gain/drive (1.0 - 10.0)
};
