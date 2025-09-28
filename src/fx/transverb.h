#pragma once

#include <cmath>
#include <vector>

#include "fx.h"

class Transverb : public Fx {
 public:
  Transverb();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void Reset();

  // Interpolation methods
  float InterpolateHermite(const std::vector<float>& data, double address);

  // Parameter processing
  void ProcessParameters();

  // Core parameters (0.0 - 1.0 range for command line)
  double bsize_param_;  // buffer size (maps to sample count)
  double speed1_;       // playback speed for buffer 1 (0.25 - 4.0)
  double speed2_;       // playback speed for buffer 2 (0.25 - 4.0)
  double drymix_;       // dry signal mix (0.0 - 1.0)
  double mix1_;         // wet mix for buffer 1 (0.0 - 1.0)
  double mix2_;         // wet mix for buffer 2 (0.0 - 1.0)
  double feed1_;        // feedback for buffer 1 (0.0 - 1.0)
  double feed2_;        // feedback for buffer 2 (0.0 - 1.0)
  double dist1_;        // stereo distribution for buffer 1 (0.0 - 1.0)
  double dist2_;        // stereo distribution for buffer 2 (0.0 - 1.0)

  // Internal state
  int bsize_;     // actual buffer size in samples
  int writer_;    // write position
  double read1_;  // read position for buffer 1
  double read2_;  // read position for buffer 2

  // Delay buffers
  std::vector<float> buf1_;
  std::vector<float> buf2_;

  // Constants
  static constexpr int MIN_BUFFER_SIZE = 1024;
  static constexpr int MAX_BUFFER_SIZE = 88200;  // 2 seconds at 44.1kHz
  static constexpr double MIN_SPEED = 0.25;
  static constexpr double MAX_SPEED = 4.0;
};
