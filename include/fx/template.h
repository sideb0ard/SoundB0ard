#pragma once

#include <defjams.h>
#include <fx/fx.h>

class Template : Fx {
 public:
  Template();
  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void Update();

  double some_setting_;
};
