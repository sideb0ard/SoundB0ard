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

  // GLOBAL SETTINGS
  double distortion_threshold{0.5};
  double volume{1};
  double pan{0};

  bool hard_sync{false};
  double detune_cents{0};
  double pulse_width_pct{50};

  // TRANSIENT
  float noise_amp{0.4};
  double noise_eg_attack_ms{1};
  double noise_eg_decay_ms{44};
  int noise_eg_mode{DIGITAL};
  unsigned int noise_filter_type{6};
  double noise_filter_fc{5000};
  double noise_filter_q{1};

  // OSCILLATORS
  int osc1_wav{SINE};
  float osc1_amp{1};
  float osc1_ratio{1};
  double osc_eg_attack_ms{1};
  double osc_eg_decay_ms{1000};
  int osc_eg_mode{ANALOG};

  // OUTPUT
  double amp_eg_attack_ms{2};
  double amp_eg_decay_ms{1000};
  int amp_eg_mode{DIGITAL};

  unsigned int amp_filter_type{6};
  double amp_filter_fc{10000};
  double amp_filter_q{1};

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

  // TRANSIENT
  std::unique_ptr<QBLimitedOscillator> noise_;
  EnvelopeGenerator noise_eg_;
  std::unique_ptr<CKThreeFive> noise_filter_;

  // PITCH
  std::unique_ptr<QBLimitedOscillator> osc1_;
  EnvelopeGenerator osc_eg_;

  // LFO lfo_;

  // OUTPUT
  EnvelopeGenerator amp_eg_;
  std::unique_ptr<CKThreeFive> amp_filter_;
  Distortion distortion_;
  double velocity_{0.};
  DCA dca_;

 private:
  // gets updated and used for calculating dur
  double ms_per_midi_tick_{0};
};

DrumSettings GetDrumSettings(std::string name);
}  // namespace SBAudio
