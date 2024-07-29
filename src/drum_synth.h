#pragma once

#include <dca.h>
#include <distortion.h>
#include <envelope_generator.h>
#include <filter_ckthreefive.h>
#include <filter_moogladder.h>
#include <lfo.h>
#include <qblimited_oscillator.h>

#include <array>
#include <memory>
#include <string>

#include "defjams.h"
#include "soundgenerator.h"

namespace SBAudio {

struct DrumSettings {
  std::string name{"Default"};

  DrumSettings() = default;
  DrumSettings(std::string, std::map<std::string, double>);

  double distortion_threshold{0.5};
  double amplitude{1};

  bool hard_sync{false};
  double detune_cents{0};
  double pulse_width_pct{50};

  // OSCILLATORS
  int osc1_wav{SINE};
  float osc1_amp{1};
  float osc1_ratio{1};
  bool filter1_enable{false};
  unsigned int filter1_type{6};
  double filter1_fc{20000};
  double filter1_q{1};

  int osc3_wav{NOISE};
  float osc3_amp{0};
  float osc3_ratio{1};
  bool filter3_enable{true};
  unsigned int filter3_type{6};
  double filter3_fc{10000};
  double filter3_q{1};

  // MASTER FILTER -> OUT
  bool master_filter_enable{false};
  unsigned int master_filter_type{6};
  double master_filter_fc{10000};
  double master_filter_q{1};

  // NOISE ENV //////////////////////////
  double noise_eg_attack_ms{1};
  double noise_eg_decay_ms{44};
  bool noise_eg_ramp_mode{true};

  // OSC ENV //////////////////////////
  double osc_eg_attack_ms{2};
  double osc_eg_decay_ms{70};
  bool osc_eg_ramp_mode{true};

  // AMP ENV //////////////////////////
  double amp_eg_attack_ms{2};
  double amp_eg_decay_ms{70};
  bool amp_eg_ramp_mode{true};

  // LFO //////////////////////////
  // int lfo_wave{SINE};
  // int lfo_mode{LFOSYNC};
  // double lfo_rate{DEFAULT_LFO_RATE};
};

DrumSettings Map2DrumSettings(std::string name,
                              std::map<std::string, double> &preset_vals);

class DrumSynth : public SoundGenerator {
 public:
  DrumSynth();
  ~DrumSynth() = default;

  std::string Info() override;
  std::string Status() override;
  StereoVal GenNext(mixer_timing_info tinfo) override;
  void Start() override;
  void Stop() override;
  void Randomize() override;
  void NoteOn(midi_event ev) override;
  void NoteOff(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  void PrintSettings(DrumSettings settings);
  void LoadPreset(std::string name,
                  std::map<std::string, double> preset_vals) override;
  void Save(std::string name) override;
  void Update();

  void LoadSettings(DrumSettings settings);

  DrumSettings settings_;

  std::unique_ptr<QBLimitedOscillator> osc1_;
  std::unique_ptr<CKThreeFive> osc1_filter_;

  std::unique_ptr<QBLimitedOscillator> noise_;
  std::unique_ptr<CKThreeFive> noise_filter_;

  EnvelopeGenerator osc_eg_;
  EnvelopeGenerator amp_eg_;
  EnvelopeGenerator noise_eg_;
  // LFO lfo_;

  Distortion distortion_;
  //  std::unique_ptr<CKThreeFive> master_filter_;

  DCA dca_;

 private:
  // gets updated and used for calculating dur
  double ms_per_midi_tick_{0};
};

DrumSettings GetDrumSettings(std::string name);
}  // namespace SBAudio
