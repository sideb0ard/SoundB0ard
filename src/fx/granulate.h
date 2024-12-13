#pragma once

#include <defjams.h>
#include <lfo.h>
#include <stdbool.h>

#include "circular_buffer.h"
#include "fx/fx.h"

class Granulate : public Fx {
 public:
  Granulate();
  ~Granulate() = default;
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  CircularBuffer<double> m_buffer{10 * SAMPLE_RATE *
                                  2};  // 10 seconds x 2 channels
  double m_wet_mix_{0.5};              // 0 - 100

  void SetWetMix(double wet_mix);
  void Update();
};
