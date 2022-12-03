#pragma once

#include <defjams.h>
#include <fx/fx.h>

class Template : Fx {
 public:
  Template();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void Update();

  double some_setting_;
};
