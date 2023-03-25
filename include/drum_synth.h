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

  int pitch_range_{30};
  double starting_frequency_{0};
  double base_frequency_{0};
  double frequency_diff_{0};

  QBLimitedOscillator osc1;
  float osc1_amp{1};
  MoogLadder filter1_;
  bool filter1_en{true};

  QBLimitedOscillator osc2;
  float osc2_amp{0};
  MoogLadder filter2_;
  bool filter2_en{true};

  EnvelopeGenerator m_eg;

  DCA m_dca;

  // Modulations and intentsities.
  bool eg_amp_{true};
  double eg_amp_int_{1};

  bool eg_o1_pitch_{true};
  double eg_o1_pitch_int_{1};

  bool eg_o2_pitch_{true};
  double eg_o2_pitch_int_{1};

  bool eg_f1_fc_{true};
  double eg_f1_fc_int_{1};

  bool eg_f1_q_{true};
  double eg_f1_q_int_{1};

  bool eg_f2_fc_{true};
  double eg_f2_fc_int_{1};

  bool eg_f2_q_{true};
  double eg_f2_q_int_{1};
};

}  // namespace SBAudio
