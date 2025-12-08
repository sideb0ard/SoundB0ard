#pragma once

#include <array>
#include <vector>

#include "fx.h"

class Nnirror : public Fx {
 public:
  Nnirror();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;
  void EventNotify(broadcast_event event, mixer_timing_info tinfo) override;

 private:
  void Init();
  void Reset();

  static constexpr int kKernelSize = 3;
  static constexpr int kMaxDelaySamples = 44100;  // 1 second at 44.1kHz

  // Circular buffer for each channel
  std::array<std::vector<float>, 2> delay_buffers_;
  std::array<int, 2> write_indices_{0, 0};

  // Blur kernel (normalized weights)
  std::array<std::array<float, kKernelSize>, kKernelSize> kernel_;

  struct DelayTap {
    int delay_samples;
    float feedback;
    float cross_feed;  // Amount to feed into opposite channel
  };
  std::vector<DelayTap> delay_taps_;

  int WrapIndex(int index) const {
    while (index < 0) index += kMaxDelaySamples;
    return index % kMaxDelaySamples;
  }

  int MsToSamples(float ms) const {
    return static_cast<int>(ms * SAMPLE_RATE / 1000.0f);
  }
};
