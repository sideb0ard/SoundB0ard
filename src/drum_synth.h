#pragma once

#include <memory>
#include <string>

#include "dca.h"
#include "defjams.h"
#include "drum_synth_modules.h"
#include "soundgenerator.h"

namespace SBAudio {

struct DrumSettings {
  std::string name{"Default"};

  DrumSettings() = default;
  DrumSettings(std::string, std::map<std::string, double>);

  // GLOBAL SETTINGS
  double volume{1};

  // 0 - BassDrum Settings
  double bd_vol{1};
  double bd_pan{0};
  double bd_tone{10000};
  double bd_q{1};
  double bd_noise_vol{0.3};
  double bd_ntone{5000};
  double bd_nq{1};
  double bd_decay{1000};
  int bd_octave{1};
  int bd_key{7};
  double bd_detune_cents{0};
  double bd_distortion_threshold{0.5};
  bool bd_hard_sync{false};

  // 1 - SnareDum Settings
  double sd_vol{1};
  double sd_pan{0};
  double sd_noise_vol{0.5};
  double sd_noise_decay{22};
  double sd_tone{1000};
  double sd_decay{50};
  double sd_osc1_freq{476};
  double sd_osc2_freq{238};

  // 2 - Closed hat Settings
  double hh_vol{1};
  double hh_pan{0};
  double hh_sqamp{0.5};
  double hh_attack{20};
  double hh_decay{10};
  double hh_midf{10000};
  double hh_hif{6000};
  double hh_hif_q{1};

  // r32 - Clap
  double cp_vol{1};
  double cp_pan{0};
  double cp_nvol{0.6};
  double cp_nattack{10};
  double cp_ndecay{207};
  double cp_tone{1000};
  double cp_fq{5};
  double cp_eg_attack{10};
  double cp_eg_decay{100};
  double cp_eg_sustain{0.3};
  double cp_eg_release{100};
  int cp_lfo_type{usaw};
  double cp_lfo_rate{5};
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

  std::unique_ptr<BassDrum> bd_;
  std::unique_ptr<SnareDrum> sd_;
  std::unique_ptr<HiHat> hh_;
  std::unique_ptr<HandClap> cp_;

  DCA dca_;
};

DrumSettings GetDrumSettings(std::string name);
}  // namespace SBAudio
