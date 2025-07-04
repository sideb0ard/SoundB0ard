#include <fx/ramp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>

namespace {
double scale(double cur_val, double cur_min, double cur_max, double new_min,
             double new_max) {
  double return_val = 0;

  double cur_range = cur_max - cur_min;
  if (cur_range == 0)
    return_val = new_min;
  else {
    double new_range = new_max - new_min;
    return_val = (((cur_val - cur_min) * new_range) / cur_range) + new_min;
  }

  return return_val;
}
}  // namespace

Ramp::Ramp() {
  Reset(SAMPLE_RATE);
}

Ramp::Ramp(double rate) {
  Reset(rate);
}

void Ramp::Reset(double frame_rate_per_second) {
  frames_per_second_ = frame_rate_per_second;
  counter_ = 0;
  signal_ = 0;
}

double Ramp::Generate() {
  signal_ = scale(counter_, 0, frames_per_second_, 0, 1);
  counter_++;
  if (counter_ == frames_per_second_) counter_ = 0;

  return signal_;
}
