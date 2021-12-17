#pragma once

#include <defjams.h>
#include <fx/fx.h>

#include <string>

class Decimate : Fx {
 public:
  Decimate();
  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;
  double GetParam(std::string name) override;

 private:
  void SetBitRes(float val);
  void SetDestruct(float val);
  void SetSampleHoldFreq(float val);
  void SetBigDivisor(float val);
  void Update();

 private:
  float bitres{1};
  float destruct{1};

  float bit1{0};
  float bit2{0};
  int samples_left{0};
  float sample_hold_freq{1};  // between 0 and 1
  float big_divisor{0.5};
};
