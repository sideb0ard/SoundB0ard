#pragma once

#include <dca.h>
#include <distortion.h>
#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <lfo.h>
#include <qblimited_oscillator.h>

#include <array>
#include <memory>
#include <string>

#include "defjams.h"
#include "soundgenerator.h"

namespace SBAudio {

// routing
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

  // MOD RANGES
  double pitch_range{30};
  double freq_range{1000};
  double q_range{3};

  // OSCILLATORS
  int osc1_wav{SINE};
  float osc1_amp{1};
  bool filter1_en{false};

  int osc2_wav{NOISE};
  float osc2_amp{0};
  bool filter2_en{false};

  // ENV //////////////////////////
  int eg_attack_ms{1};
  int eg_decay_ms{0};
  int eg_sustain_level{1};
  int eg_release_ms{70};
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
  void Save(std::string name) override;
  void ListPresets() override;

  std::string patch_name_{"Default"};

  // oscillation ranges
  double pitch_range_{30};
  double freq_range_{1000};
  double q_range_{3};

  // only used for pitch
  double starting_frequency_{0};
  double base_frequency_{0};
  double frequency_diff_{0};

  std::unique_ptr<QBLimitedOscillator> osc1_;
  float osc1_amp_{1};

  std::unique_ptr<MoogLadder> filter1_;
  bool filter1_en_{false};

  std::unique_ptr<QBLimitedOscillator> osc2_;
  float osc2_amp_{0};

  std::unique_ptr<MoogLadder> filter2_;
  bool filter2_en_{false};

  EnvelopeGenerator eg_;
  LFO lfo_;

  Distortion distortion_;

  DCA dca_;

  std::array<std::array<int, NUM_DESTD>, NUM_SRCI> modulations_{{}};
};

}  // namespace SBAudio
