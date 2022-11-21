#ifndef LOOPER_H
#define LOOPER_H

#include <envelope_generator.h>
#include <soundgenerator.h>
#include <stdbool.h>

namespace SBAudio {

constexpr int kMaxConcurrentGrains = 1000;

struct SoundGrainParams {
  float dur_frames{0};
  int starting_idx{0};
  int attack_pct{0};
  int release_pct{0};
  bool reverse_mode{0};
  double pitch{0};
  int num_channels{0};
  int degrade_by{0};
  bool debug{false};
  unsigned int envelope_mode{0};
  int dur_ms{0};
};

struct SoundGrain {
  SoundGrain() = default;
  ~SoundGrain() = default;

  stereo_val Generate(std::vector<double> &audio_buffer);
  void Initialize(SoundGrainParams);

  int grain_len_frames{0};
  int grain_counter_frames{0};
  int audiobuffer_num{0};
  int audiobuffer_start_idx{0};
  int audiobuffer_num_channels{0};
  double audiobuffer_cur_pos{0};
  double audiobuffer_pitch{0};
  double incr{0};

  int degrade_by{0};

  bool active{false};
  bool reverse_mode{false};
  bool debug{false};

  EnvelopeGenerator eg;
  // double amp{0};
  // double amp_increment;
  int start_sustain_frame{0};
  int release_frame{0};
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
  stereo_val GenNext(mixer_timing_info tinfo) override;
  std::string Status() override;
  std::string Info() override;
  void start() override;
  void stop() override;
  void eventNotify(broadcast_event event, mixer_timing_info tinfo) override;
  void noteOn(midi_event ev) override;
  void noteOff(midi_event ev) override;
  void SetParam(std::string name, double val) override;

 public:
  bool started{false};

  std::string filename;
  std::vector<double> audio_buffer{};
  int num_channels{0};
  int size_of_sixteenth{0};
  int audio_buffer_read_idx{0};

  int num_active_grains{0};
  int highest_grain_num{0};
  int cur_grain_num{0};
  std::array<SoundGrain, kMaxConcurrentGrains> m_grains{};

  int granular_spray_frames{0};  // random off-set from starting idx
  int quasi_grain_fudge{0};      // random variation from length of grain
  int grain_duration_ms{0};
  int grains_per_sec{0};
  bool density_duration_sync{false};  // keep duration and per_sec aligned
  double fill_factor{0};              // used for density_duration_sync
  double grain_pitch{1};

  int num_grains_per_looplen{0};
  unsigned int envelope_mode{0};
  bool reverse_mode{false};

  int next_grain_launch_sample_time{0};
  int grain_attack_time_pct{0};
  int grain_release_time_pct{0};
  int attack_frames_{0};

  EnvelopeGenerator m_eg1;  // start/stop amp

  LoopMode loop_mode_{LoopMode::loop_mode};
  double loop_len{1};  // bars
  int loop_counter{-1};

  bool scramble_pending{false};
  bool scramble_mode{false};
  int scramble_idx{0};

  bool stutter_pending{false};
  bool stutter_mode{false};
  int stutter_idx{0};

  bool stop_pending{false};  // allow eg to stop
  bool gate_mode{false};     // use midi to trigger env amp

  int degrade_by{0};  // percent change to drop bits

  int cur_sixteenth{0};  // used to track scramble

  double incr_speed_{1};
  double cur_midi_idx_{-1};
  int cur_frame_tick{-1};
  int number_of_frames{0};
  float idx_incr{0};

  double grain_spacing{0};

  bool debug_pending{false};

 public:
  void ImportFile(std::string filename);
  void SetGateMode(bool b);

  // int CalculateGrainSpacing(mixer_timing_info tinfo);
  void SetGrainPitch(double pitch);
  void SetIncrSpeed(double speed);
  void SetGrainDuration(int dur);
  void SetGrainDensity(int gps);
  void SetGrainAttackSizePct(int att);
  void SetGrainReleaseSizePct(int rel);
  void SetAudioBufferReadIdx(int position);
  void SetGranularSpray(int spray_ms);
  void SetQuasiGrainFudge(int fudgefactor);
  void SetSelectionMode(unsigned int mode);
  void SetEnvelopeMode(unsigned int mode);
  void SetReverseMode(bool b);
  void SetLoopMode(unsigned int m);
  void SetLoopLen(double bars);
  void SetScramblePending();
  void SetStutterPending();

  int GetAvailableGrainNum();
  int CountActiveGrains();
  void LaunchGrain(mixer_timing_info tinfo);

  void SetFillFactor(double fill_factor);
  void SetDensityDurationSync(bool b);

  void SetGrainEnvAttackPct(int percent);
  void SetGrainEnvReleasePct(int percent);
  void SetGrainExternalSourceMode(unsigned int mode);
  void SetDegradeBy(int degradation);
  void SetTraceEnvelope();
};

}  // namespace SBAudio
#endif  // LOOPER
