#ifndef LOOPER_H
#define LOOPER_H

#include <envelope_generator.h>
#include <fx/ramp.h>
#include <soundgenerator.h>
#include <stdbool.h>

namespace SBAudio {

struct SoundGrainParams {
  int dur_frames{0};
  int starting_idx{0};
  bool reverse_mode{0};
  double pitch{0};
  int num_channels{0};
  int degrade_by{0};
};

struct SoundGrain {
  SoundGrain() = default;
  ~SoundGrain() = default;

  StereoVal Generate(std::vector<double> &audio_buffer);
  void Initialize(SoundGrainParams);

  int grain_len_frames{0};
  int grain_frame_counter{0};

  int audiobuffer_num_channels{0};

  double audiobuffer_cur_pos{0};
  double audiobuffer_pitch{0};
  double incr{0};

  int degrade_by{0};

  bool active{false};
  bool reverse_mode{false};
};

enum LoopMode {
  loop_mode,
  static_mode,
  smudge_mode,
};

class Looper : public SoundGenerator {
 public:
  Looper(std::string filename, unsigned int loop_mode);
  ~Looper();
  StereoVal GenNext(mixer_timing_info tinfo) override;
  std::string Status() override;
  std::string Info() override;
  void start() override;
  void stop() override;
  void EventNotify(broadcast_event event, mixer_timing_info tinfo) override;
  void noteOn(midi_event ev) override;
  void noteOff(midi_event ev) override;
  void SetParam(std::string name, double val) override;

 public:
  bool started_{false};

  std::string filename_;
  std::vector<double> audio_buffer_{};
  int num_channels_{0};
  int size_of_sixteenth_{0};
  int audio_buffer_read_idx_{0};

  SoundGrain grain_a_;
  SoundGrain grain_b_;

  SoundGrain *active_grain_;
  SoundGrain *incoming_grain_;

  int granular_spray_frames_{0};  // random off-set from starting idx
  int quasi_grain_fudge_{0};      // random variation from length of grain

  int grains_per_sec_{0};
  int grain_duration_frames_{0};
  int grain_spacing_frames_{0};

  double grain_pitch_{1};

  bool reverse_mode_{false};
  bool reverse_pending_{false};

  Ramp xfader_;
  void SwitchXFadeGrains();
  int grain_ramp_time_{0};
  int next_grain_launch_sample_time_{0};
  int start_xfade_at_frame_time_{0};
  int stop_xfade_at_frame_time_{0};
  int xfade_time_in_frames_{0};
  bool xfader_active_{false};

  EnvelopeGenerator eg_;  // start/stop amp

  LoopMode loop_mode_{LoopMode::loop_mode};
  double loop_len_{1};  // bars
  int loop_counter_{-1};

  bool scramble_mode_{false};
  bool scramble_pending_{false};
  int current_sixteenth_{0};

  bool stutter_mode_{false};
  bool stutter_pending_{false};
  int stutter_idx_{0};

  bool stop_pending_{false};  // allow eg to stop

  int degrade_by_{0};  // percent change to drop bits

  int cur_sixteenth_{0};  // used to track scramble

  double incr_speed_{1};
  double cur_midi_idx_{0};

  double plooplen_{16};
  double poffset_{0};
  int pinc_{1};
  bool pbounce_{false};
  bool preverse_{false};

 public:
  void ImportFile(std::string filename);

  void SetGrainPitch(double pitch);
  void SetIncrSpeed(double speed);
  void SetGrainDuration(int dur);
  void SetGrainDensity(int gps);
  void SetAudioBufferReadIdx(size_t position);
  void SetGranularSpray(int spray_ms);
  void SetQuasiGrainFudge(int fudgefactor);
  void SetReverseMode(bool b);
  void SetLoopMode(unsigned int m);
  void SetLoopLen(double bars);
  void SetScramblePending();
  void SetStutterPending();
  void SetReversePending();

  void LaunchGrain(SoundGrain *grain, mixer_timing_info tinfo);

  void SetGrainSlopePct(int percent);
  void SetDegradeBy(int degradation);

  void SetPOffset(int poffset);
  void SetPlooplen(int plooplen);
  void SetPinc(int pinc);
};

}  // namespace SBAudio
#endif  // LOOPER
