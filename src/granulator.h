#pragma once

#include <stdbool.h>

#include <memory>

#include "envelope_generator.h"
#include "fx/ramp.h"
#include "grains.h"
#include "soundgenerator.h"
#include "stepper.h"

namespace SBAudio {

class Granulator : public SoundGenerator {
 public:
  Granulator(std::string filename, unsigned int loop_mode);
  Granulator();
  ~Granulator() override;
  StereoVal GenNext(mixer_timing_info tinfo) override;
  std::string Status() override;
  std::string Info() override;
  void Start() override;
  void Stop() override;
  void EventNotify(broadcast_event event, mixer_timing_info tinfo) override;
  void NoteOn(midi_event ev) override;
  void NoteOff(midi_event ev) override;
  void AllNotesOff() override;
  void SetParam(std::string name, double val) override;

  void AddBuffer(std::unique_ptr<FileBuffer> fb) override;
  void SetSubParam(int id, std::string name, double val) override;

  void Reset();

 public:
  bool started_{false};

  std::unique_ptr<FileBuffer> file_buffer_;

  SoundGrainType grain_type_{SoundGrainType::Sample};
  std::unique_ptr<SoundGrain> grain_a_;
  std::unique_ptr<SoundGrain> grain_b_;

  SoundGrain *active_grain_;
  SoundGrain *incoming_grain_;

  int granular_spray_frames_{0};  // random off-set from starting idx
  int quasi_grain_fudge_{0};      // random variation from length of grain

  int grains_per_sec_{0};
  int grain_duration_frames_{0};
  int grain_spacing_frames_{0};

  bool reverse_mode_{false};
  bool reverse_pending_{false};

  Ramp xfader_;
  void SwitchXFadeGrains();
  int grain_ramp_time_{0};
  int next_grain_launch_sample_time_{1};
  int start_xfade_at_frame_time_{0};
  int stop_xfade_at_frame_time_{0};
  int xfade_time_in_frames_{0};
  bool xfader_active_{false};

  EnvelopeGenerator eg_;  // start/stop amp

  bool stop_count_pending_{false};
  int stop_len_{0};
  int stop_countr_{0};

  bool stop_pending_{false};  // allow eg to stop
  int degrade_by_{0};         // percent change to drop bits
                              //

 public:
  void SetPitch(double pitch_ratio);
  void SetIncrSpeed(double speed);
  void SetGrainDuration(int dur);
  void SetGrainDensity(int gps);
  void SetGranularSpray(int spray_ms);
  void SetQuasiGrainFudge(int fudgefactor);
  void SetReverseMode(bool b);

  void SetLoopMode(unsigned int m);
  void SetLoopLen(double bars);
  void SetScramblePending();
  void SetStopPending(int loops);
  void SetStutterPending();
  void SetReversePending();

  void LaunchGrain(SoundGrain *grain, mixer_timing_info tinfo);

  void SetGrainSlopePct(int percent);
  void SetDegradeBy(int degradation);

  void SetPidx(int val);
  void SetPOffset(int poffset);
  void SetPlooplen(int plooplen);
  void SetPinc(int pinc);
};

}  // namespace SBAudio
