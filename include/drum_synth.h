#pragma once

#include <dca.h>
#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <qblimited_oscillator.h>
#include <soundgenerator.h>

static const char DRUM_SYNTH_PATCHES[] = "settings/drumpresets.dat";

class DrumSynth : public SoundGenerator {
 public:
  DrumSynth();
  ~DrumSynth() = default;

  std::string Info() override;
  std::string Status() override;
  stereo_val genNext(mixer_timing_info tinfo) override;
  void start() override;
  void noteOn(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  double GetParam(std::string name) override;
  void Load(std::string name) override;
  void Save(std::string name) override;
  void ListPresets() override;

  std::string patch_name{};

  QBLimitedOscillator osc1;
  float osc1_amp{1};

  QBLimitedOscillator osc2;
  float osc2_amp{0};

  EnvelopeGenerator amp_env;
  bool amp_env_to_osc1{false};
  bool amp_env_to_osc2{false};
  float amp_env_int{1};

  EnvelopeGenerator pitch_env;
  bool pitch_env_to_osc1{true};
  bool pitch_env_to_osc2{false};
  float pitch_env_int{0.75};

  MoogLadder filter1;
  bool f1_osc1_enable{false};
  bool f1_osc2_enable{false};
  MoogLadder filter2;
  bool f2_osc1_enable{false};
  bool f2_osc2_enable{false};

  DCA m_dca;
};
