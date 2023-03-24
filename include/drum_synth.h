#pragma once

#include <dca.h>
#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <qblimited_oscillator.h>
#include <soundgenerator.h>

namespace SBAudio {

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
  void Load(std::string name) override;
  void Save(std::string name) override;
  void ListPresets() override;

  std::string patch_name{};

  int pitch_range_{4};
  double starting_frequency_{0};
  double base_frequency_{0};
  double frequency_diff_{0};

  QBLimitedOscillator osc1;
  float osc1_amp{1};

  QBLimitedOscillator osc2;
  float osc2_amp{0};

  EnvelopeGenerator m_eg;

  MoogLadder filter_;

  DCA m_dca;
};

}  // namespace SBAudio
