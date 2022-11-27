#pragma once
#include <defjams.h>

class Ramp {
 public:
  Ramp();
  Ramp(double rate);
  double Generate();
  void Reset(double frames_rate_per_second);

  int64_t counter_{0};
  int64_t frames_per_second_{0};

  double signal_{0};  // return value for generate.
};
