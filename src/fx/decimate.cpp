#include <fx/decimate.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <iostream>
#include <sstream>

namespace {
float quantize(float old, float q) {
  if (q >= 1.0) return old;

  /* favor low end, more exciting there */
  old = old * old * old;

  long scale = (long)(512 * q);
  if (scale == 0) scale = 2;
  long X = (long)(scale * old);

  return (float)(X / (float)scale);
}

inline float destroy(float q, float old) {
  if (old >= 0.999999) return q;
  if (q <= 0.000001) return 1.0;

  unsigned long scale = (unsigned long)(65536 * q);

  unsigned long X = (unsigned long)(scale * old);

  return (float)(X / (float)scale);
}

}  // namespace

Decimate::Decimate() {
  type_ = DECIMATE;
  enabled_ = true;
}

std::string Decimate::Status() {
  std::stringstream ss;
  ss << "bitres:" << bitres;
  ss << " destruct:" << destruct;
  ss << " sample_hold_freq:" << sample_hold_freq;
  ss << " big_divisor:" << big_divisor;
  return ss.str();
}

StereoVal Decimate::Process(StereoVal input) {
  if (samples_left--) {
    return input;
  } else {
    bit1 = input.left;
    bit2 = input.right;
    samples_left =
        (int)((1 + (big_divisor * 1024.)) * (1.0 - sample_hold_freq));
  }

  StereoVal output;
  bit1 += destroy(quantize(bit1, bitres), destruct);  // accumulating
  bit2 += destroy(quantize(bit2, bitres), destruct);

  if (bit1 > 1 || bit2 > 1) {
    std::cout << "OVERLOAD!: b1:" << bit1 << " b2:" << bit2 << std::endl;
    return input;
  }
  output.left = bit1;
  output.right = bit2;

  return output;
}

void Decimate::SetParam(std::string name, double val) {
  if (name == "bitres")
    SetBitRes(val);
  else if (name == "destruct")
    SetDestruct(val);
  else if (name == "sample_hold_freq")
    SetSampleHoldFreq(val);
  else if (name == "big_divisor")
    SetBigDivisor(val);
  Update();
}


void Decimate::Update() {}

void Decimate::SetBitRes(float val) {
  if (val >= 1 && val <= 16) {
    bitres = val;
    Update();
  } else
    printf("Val must be between 1 and 16:%f\n", val);
}

void Decimate::SetSampleHoldFreq(float val) {
  val = clamp(0, 1, val);
  sample_hold_freq = val;
  Update();
}

void Decimate::SetDestruct(float val) {
  if (val > 0 && val <= 1) {
    destruct = val;
    Update();
  } else
    printf("Val must be between 0 and 1\n");
}

void Decimate::SetBigDivisor(float val) {
  if (val > 0 && val <= 1) {
    Update();
  } else
    printf("Val must be between 0 and 1\n");
}
