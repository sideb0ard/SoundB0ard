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
  double bd_noise_enabled{false};
  double bd_noise_vol{0.3};
  double bd_ntone{10000};
  double bd_nq{1};
  double bd_decay{180};
  int bd_octave{2};
  int bd_key{40};
  double bd_detune_cents{0};
  bool bd_use_distortion{true};
  double bd_distortion_threshold{0.5};
  bool bd_hard_sync{false};
  bool bd_use_delay{false};
  int bd_delay_mode{0};  // 0 - norm, 1 - tap1, 2 - tap2, 3 - pingpong
  double bd_delay_ms{23};
  double bd_delay_feedback_pct{0};
  double bd_delay_ratio{0};
  double bd_delay_wetmix{0.5};
  bool bd_delay_sync_tempo{true};
  int bd_delay_sync_len{0};  // 0 none, 1 - 1/4 // 2 - 8th // 3 - 16th

  // 1 - SnareDum Settings
  double sd_vol{1};
  double sd_pan{0};
  double sd_noise_vol{0.5};
  double sd_noise_decay{22};
  double sd_tone{1000};
  double sd_decay{50};
  int sd_octave{3};
  int sd_key{7};
  int sd_hi_osc_waveform{0};
  int sd_lo_osc_waveform{0};
  double sd_distortion_threshold{0.5};
  bool sd_use_delay{false};
  int sd_delay_mode{0};  // 0 - norm, 1 - tap1, 2 - tap2, 3 - pingpong
  double sd_delay_ms{23};
  double sd_delay_feedback_pct{0};
  double sd_delay_ratio{0};
  double sd_delay_wetmix{0.5};
  bool sd_delay_sync_tempo{true};
  int sd_delay_sync_len{0};  // 0 none, 1 - 1/4 // 2 - 8th // 3 - 16th

  // 2 - Closed hat Settings
  double hh_vol{1};
  double hh_pan{0};
  double hh_sqamp{0.5};
  double hh_attack{20};
  double hh_decay{10};
  double hh_midf{10000};
  double hh_hif{6000};
  double hh_hif_q{1};
  double hh_distortion_threshold{0.5};
  bool hh_use_delay{false};
  int hh_delay_mode{0};  // 0 - norm, 1 - tap1, 2 - tap2, 3 - pingpong
  double hh_delay_ms{23};
  double hh_delay_feedback_pct{0};
  double hh_delay_ratio{0};
  double hh_delay_wetmix{0.5};
  bool hh_delay_sync_tempo{true};
  int hh_delay_sync_len{0};  // 0 none, 1 - 1/4 // 2 - 8th // 3 - 16th

  // 3 - Clap
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
  double cp_distortion_threshold{0.5};
  bool cp_use_delay{false};
  int cp_delay_mode{0};  // 0 - norm, 1 - tap1, 2 - tap2, 3 - pingpong
  double cp_delay_ms{23};
  double cp_delay_feedback_pct{0};
  double cp_delay_ratio{0};
  double cp_delay_wetmix{0.5};
  bool cp_delay_sync_tempo{true};
  int cp_delay_sync_len{0};  // 0 none, 1 - 1/4 // 2 - 8th // 3 - 16th
                             //
  // 4 - Open hat Settings
  double hh2_vol{1};
  double hh2_pan{0};
  double hh2_sqamp{0.5};
  double hh2_attack{20};
  double hh2_decay{100};
  double hh2_midf{10000};
  double hh2_hif{6000};
  double hh2_hif_q{1};
  double hh2_distortion_threshold{0.5};
  bool hh2_use_delay{false};
  int hh2_delay_mode{0};  // 0 - norm, 1 - tap1, 2 - tap2, 3 - pingpong
  double hh2_delay_ms{13};
  double hh2_delay_feedback_pct{0};
  double hh2_delay_ratio{0};
  double hh2_delay_wetmix{0.5};
  bool hh2_delay_sync_tempo{true};
  int hh2_delay_sync_len{0};  // 0 none, 1 - 1/4 // 2 - 8th // 3 - 16th

  // 5 - FM1
  double fm1_vol{0.4};
  double fm1_pan{-0.1};
  double fm1_carrier_freq{43};
  double fm1_carrier_eg_attack{10};
  double fm1_carrier_eg_decay{1};
  double fm1_carrier_eg_sustain{0.2};
  double fm1_carrier_eg_release{10};
  double fm1_modulator_freq_ratio{13};
  double fm1_modulator_eg_attack{15};
  double fm1_modulator_eg_decay{15};
  double fm1_modulator_eg_sustain{0.5};
  double fm1_modulator_eg_release{150};
  // 6 - FM2
  double fm2_vol{0.4};
  double fm2_pan{0.2};
  double fm2_carrier_freq{66};
  double fm2_carrier_eg_attack{3};
  double fm2_carrier_eg_decay{20};
  double fm2_carrier_eg_sustain{0.5};
  double fm2_carrier_eg_release{100};
  double fm2_modulator_freq_ratio{7.03};
  double fm2_modulator_eg_attack{10};
  double fm2_modulator_eg_decay{70};
  double fm2_modulator_eg_sustain{0.5};
  double fm2_modulator_eg_release{80};
  // 7 - FM3
  double fm3_vol{0.4};
  double fm3_pan{0.1};
  double fm3_carrier_freq{65.4};
  double fm3_carrier_eg_attack{19};
  double fm3_carrier_eg_decay{90};
  double fm3_carrier_eg_sustain{0.2};
  double fm3_carrier_eg_release{90};
  double fm3_modulator_freq_ratio{3.4};
  double fm3_modulator_eg_attack{10};
  double fm3_modulator_eg_decay{10};
  double fm3_modulator_eg_sustain{0.5};
  double fm3_modulator_eg_release{180};

  // 8 - Lazer
  double lz_vol{0.7};
  double lz_pan{0};
  double lz_freq{220};
  double lz_attack{10};
  double lz_decay{180};
  double lz_osc_range{47};
};

DrumSettings Map2DrumSettings(std::string name,
                              std::map<std::string, double> &preset_vals);

class DrumSynth : public SoundGenerator {
 public:
  DrumSynth();
  ~DrumSynth() override = default;

  std::string Info() override;
  std::string Status() override;
  StereoVal GenNext(mixer_timing_info tinfo) override;
  void Start() override;
  void Stop() override;
  void Randomize() override;
  void NoteOn(midi_event ev) override;
  void NoteOff(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  void SetVolume(double v) override;
  void SetPan(double p) override;
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
  std::unique_ptr<HiHat> hh2_;
  std::unique_ptr<HandClap> cp_;
  std::unique_ptr<FMDrum> fm1_;
  std::unique_ptr<FMDrum> fm2_;
  std::unique_ptr<FMDrum> fm3_;
  std::unique_ptr<Lazer> lz_;

  DCA dca_;
};

DrumSettings GetDrumSettings(std::string name);
}  // namespace SBAudio
