#pragma once

#include "oscillator.h"

struct QBLimitedOscillator : public Oscillator {
  QBLimitedOscillator() = default;
  ~QBLimitedOscillator() override = default;

  double DoOscillate(double *quad_phase_output) override;

  void StartOscillator() override;
  void StopOscillator() override;

  void Reset() override;

  double DoSawtooth(double modulo, double dInc);
  double DoSquare(double modulo, double dInc);
  double DoTriangle(double modulo, double dInc, double dFo,
                    double dSquareModulator, double *pZ_register);
};
