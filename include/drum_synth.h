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

enum SourceIdx { ENVI, LFOI, NUM_SRCI };
enum DestIdx {
  OSC1_PITCHD,
  OSC2_PITCHD,
  FILTER1_FCD,
  FILTER1_QD,
  FILTER2_FCD,
  FILTER2_QD,
  NUM_DESTD
};

struct DrumSettings {
  std::string name{"Default"};

  double distortion_threshold{0.5};
  double amplitude{1};

  // MOD RANGES
  double pitch_range{30};
  double q_range{3};

  // used for pitch modulation
  double starting_frequency{0};
  double base_frequency{0};
  double frequency_diff{0};

  // OSCILLATORS
  int osc1_wav{SINE};
  float osc1_amp{1};
  bool filter1_en{false};
  unsigned int filter1_type{6};
  double filter1_fc{10000};
  double filter1_q{1};

  int osc2_wav{NOISE};
  float osc2_amp{0};
  bool filter2_en{true};
  unsigned int filter2_type{6};
  double filter2_fc{10000};
  double filter2_q{1};

  // ENV //////////////////////////
  int eg_attack_ms{1};
  int eg_decay_ms{0};
  int eg_sustain_level{1};
  int eg_release_ms{70};
  int eg_hold_time_ms{0};
  bool eg_ramp_mode{true};

  // LFO //////////////////////////
  int lfo_wave{SINE};
  int lfo_mode{LFOSYNC};
  double lfo_rate{DEFAULT_LFO_RATE};

  // ROUTINGS /////////////////////
  //////////////////////////////////////

  std::array<std::array<int, NUM_DESTD>, NUM_SRCI> modulations{{}};
};
static const char DRUM_SYNTH_PATCHES[] = "settings/drumpresets.dat";

class DrumSynth : public SoundGenerator {
 public:
  DrumSynth();
  ~DrumSynth() = default;

  std::string Info() override;
  std::string Status() override;
  StereoVal GenNext(mixer_timing_info tinfo) override;
  void start() override;
  void stop() override;
  void noteOn(midi_event ev) override;
  void noteOff(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  void LoadSettings(DrumSettings settings);
  void Load(std::string name) override;
  void Save(std::string name) override;
  void ListPresets() override;

  DrumSettings settings_;

  std::unique_ptr<QBLimitedOscillator> osc1_;
  // std::unique_ptr<MoogLadder> filter1_;
  std::unique_ptr<CKThreeFive> filter1_;

  std::unique_ptr<QBLimitedOscillator> osc2_;
  // std::unique_ptr<MoogLadder> filter2_;
  std::unique_ptr<CKThreeFive> filter2_;

  EnvelopeGenerator eg_;
  LFO lfo_;

  Distortion distortion_;

  DCA dca_;

 private:
  // gets updated and used for calculating dur
  double ms_per_midi_tick_{0};
};

DrumSettings GetDrumSettings(std::string name);
}  // namespace SBAudio
