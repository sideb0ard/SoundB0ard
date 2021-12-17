#pragma once

#include <defjams.h>
#include <fx/fx.h>

class Distortion : Fx {
 public:
  Distortion();

  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;
  double GetParam(std::string name) override;

 private:
  void Init();
  void SetThreshold(double val);

 private:
  double m_threshold_;
};
