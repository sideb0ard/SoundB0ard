#pragma once

#include <defjams.h>
#include <fx/fx.h>

class Distortion : public Fx {
 public:
  Distortion();

  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void SetThreshold(double val);

 public:
  double m_threshold_;
};
