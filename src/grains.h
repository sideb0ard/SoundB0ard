#pragma once

#include <vector>

#include "defjams.h"

namespace SBAudio {

enum SoundGrainType {
  Sample,
  Signal,
};

struct SoundGrainParams {
  SoundGrainType grain_type;
  int dur_frames{0};
  int starting_idx{0};
  bool reverse_mode{0};
  int num_channels{0};
  int degrade_by{0};
  double pitch_scale{1};
  std::vector<double> *audio_buffer;
};

struct SoundGrain {
  virtual StereoVal Generate() = 0;
  virtual void Initialize(SoundGrainParams) = 0;
  virtual void SetReadIdx(int idx) = 0;
  virtual ~SoundGrain() = default;
  SoundGrainType type;
};

struct SoundGrainSample : public SoundGrain {
  SoundGrainSample() = default;
  ~SoundGrainSample() = default;

  SoundGrainType grain_type_{SoundGrainType::Sample};

  StereoVal Generate() override;
  void Initialize(SoundGrainParams) override;
  void SetReadIdx(int idx) override;

  int grain_len_frames{0};
  int grain_frame_counter{0};

  int audiobuffer_num_channels{0};

  std::vector<double> *audio_buffer;
  double audiobuffer_cur_pos{0};
  double pitch_scale{1};
  double incr{1};
  std::vector<double> pitched_audio_buffer{0};

  int degrade_by{0};

  bool active{false};
  bool reverse_mode{false};
};

}  // namespace SBAudio
