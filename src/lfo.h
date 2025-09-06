#pragma once

#include "oscillator.h"

struct LFO : public Oscillator {
  LFO();
  ~LFO() override = default;

  virtual void StartOscillator() override;
  virtual void StopOscillator() override;
  virtual double DoOscillate(double *aux_output) override;
};
