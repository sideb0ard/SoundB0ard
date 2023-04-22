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
  double q_range{7};

  // OSCILLATORS
  int osc1_wav{SINE};
  float osc1_amp{1};
  float osc1_ratio{1};
  bool filter1_enable{false};
  unsigned int filter1_type{6};
  double filter1_fc{10000};
  double filter1_q{1};

  int osc2_wav{NOISE};
  float osc2_amp{0};
  float osc2_ratio{1};
  bool filter2_enable{true};
  unsigned int filter2_type{6};
  double filter2_fc{10000};
  double filter2_q{1};

  // MASTER FILTER -> OUT
  bool master_filter_enable{false};
  unsigned int master_filter_type{6};
  double master_filter_fc{10000};
  double master_filter_q{1};

  // ENV //////////////////////////
  double eg_attack_ms{1};
  double eg_decay_ms{0};
  double eg_sustain_level{1};
  double eg_release_ms{70};
  double eg_hold_time_ms{0};
  bool eg_ramp_mode{true};

  // LFO //////////////////////////
  int lfo_wave{SINE};
  int lfo_mode{LFOSYNC};
  double lfo_rate{DEFAULT_LFO_RATE};

  // ROUTINGS /////////////////////
  //////////////////////////////////////

  // EG ->
  bool eg_osc1_pitch_enable{false};
  bool eg_osc2_pitch_enable{false};
  bool eg_filter1_freq_enable{false};
  bool eg_filter1_q_enable{false};
  bool eg_filter2_freq_enable{false};
  bool eg_filter2_q_enable{false};
  bool eg_master_filter_freq_enable{false};
  bool eg_master_filter_q_enable{false};
  // LFO ->
  bool lfo_osc1_pitch_enable{false};
  bool lfo_osc2_pitch_enable{false};
  bool lfo_filter1_freq_enable{false};
  bool lfo_filter1_q_enable{false};
  bool lfo_filter2_freq_enable{false};
  bool lfo_filter2_q_enable{false};
  bool lfo_master_filter_freq_enable{false};
  bool lfo_master_filter_q_enable{false};
  bool lfo_master_amp_enable{false};
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
  void randomize() override;
  void noteOn(midi_event ev) override;
  void noteOff(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  void LoadSettings(DrumSettings settings);
  void PrintSettings(DrumSettings settings);
  void Load(std::string name) override;
  void Save(std::string name) override;
  void ListPresets() override;
  void Update();

  DrumSettings settings_;

  // used for pitch modulation
  double starting_frequency_{0};
  double base_frequency_{0};
  double frequency_diff_{0};

  std::unique_ptr<QBLimitedOscillator> osc1_;
  std::unique_ptr<MoogLadder> filter1_;
  // std::unique_ptr<CKThreeFive> filter1_;

  std::unique_ptr<QBLimitedOscillator> osc2_;
  std::unique_ptr<MoogLadder> filter2_;
  // std::unique_ptr<CKThreeFive> filter2_;

  EnvelopeGenerator eg_;
  LFO lfo_;

  Distortion distortion_;
  std::unique_ptr<MoogLadder> master_filter_;

  DCA dca_;

 private:
  // gets updated and used for calculating dur
  double ms_per_midi_tick_{0};
};

DrumSettings GetDrumSettings(std::string name);
}  // namespace SBAudio
