#include <fx/ramp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <iostream>
#include <sstream>

Ramp::Ramp() { Reset(SAMPLE_RATE); }

Ramp::Ramp(double rate) { Reset(rate); }

void Ramp::Reset(double frames_rate_per_second) {
  frames_per_second_ = frames_rate_per_second;
  counter_ = 0;
  signal_ = 0;
}

double Ramp::Generate() {
  signal_ = scale(counter_, 0, frames_per_second_, 0, 1);
  counter_++;
  if (counter_ == frames_per_second_) counter_ = 0;

  return signal_;
}
