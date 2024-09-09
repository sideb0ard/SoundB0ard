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
  int bd_octave{2};
  int bd_key{40};
  double bd_detune_cents{0};
  bool bd_distortion_enabled{false};
  double bd_distortion_threshold{0.5};
  bool bd_hard_sync{false};
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
  int hh2_delay_mode{0};  // 0 - norm, 1 - tap1, 2 - tap2, 3 - pingpong
  double hh2_delay_ms{13};
  double hh2_delay_feedback_pct{0};
  double hh2_delay_ratio{0};
  double hh2_delay_wetmix{0.5};
  bool hh2_delay_sync_tempo{true};
  int hh2_delay_sync_len{0};  // 0 none, 1 - 1/4 // 2 - 8th // 3 - 16th

  // 5 - Low Conga / Tom
  double lt_vol{0.7};
  double lt_pan{0};
  double lt_tuning{0};
  bool lt_is_conga{false};
  // 6 - Mid Conga / Tom
  double mt_vol{0.7};
  double mt_pan{0};
  double mt_tuning{0};
  bool mt_is_conga{false};
  // 7 - High Conga / Tom
  double ht_vol{0.7};
  double ht_pan{0};
  double ht_tuning{0};
  bool ht_is_conga{false};
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
  std::unique_ptr<HiHat> hh2_;
  std::unique_ptr<HandClap> cp_;
  std::unique_ptr<TomConga> lt_;
  std::unique_ptr<TomConga> mt_;
  std::unique_ptr<TomConga> ht_;

  DCA dca_;
};

DrumSettings GetDrumSettings(std::string name);
}  // namespace SBAudio
