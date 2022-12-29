#pragma once

#include <defjams.h>
#include <fx/fx.h>

class GenZ : public Fx {
 public:
  GenZ();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;
  void EventNotify(broadcast_event event, mixer_timing_info tinfo) override;

 private:
  void Reset();

  int64_t counter_{0};
  int64_t loop_len_in_frames_{0};
  double signal_{0};
  double rate_{1};
};
