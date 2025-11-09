#pragma once

#include <map>
#include <set>
#include <string>

#include "defjams.h"
#include "ramp.h"

class XFader {
 public:
  XFader() = default;
  ~XFader() = default;

  void Assign(unsigned int left_or_right, int soundgenerator_idx);
  void Clear();

  void Update(mixer_timing_info tinfo);
  double GetValueFor(int soundgenerator_idx);
  void Set(std::string var, double val);
  std::string Status(const std::map<int, std::string> &sg_names);

 private:
  void SetXFaderPosition(double pos);  // between -1 and 1;
  void SetFadeTimeMidiTicks(double tics);
  std::set<int> left_channel_{};
  std::set<int> right_channel_{};

  int frames_per_midi_tick_{0};

  double xfade_time_midi_tics_{1};
  bool active_fade_{false};
  unsigned int fading_direction_{LEFT};

  double xfader_position_{0};  // from -1 to 1
  double xfader_destination_position_{0};
  double increment_{0};
};
