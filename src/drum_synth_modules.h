#include <qblimited_oscillator.h>

#include <array>
#include <memory>
#include <vector>

#include "dca.h"
#include "defjams.h"
#include "distortion.h"
#include "envelope_generator.h"
#include "filters/filter_ckthreefive.h"
#include "filters/filter_moogladder.h"
#include "filters/filter_sem.h"
#include "fx/stereodelay.h"
#include "lfo.h"
#include "qblimited_oscillator.h"

namespace SBAudio {

// SquareOscillatorBank used in the HiHat module below

const int kNumOscillators{6};
const std::array<float, kNumOscillators> kOscFrequencies{263, 400, 421,
                                                         474, 587, 845};
// const std::array<float, kNumOscillators> kOscFrequencies{80,    120,   166.4,
//                                                          217.2, 271.6,
//                                                          328.4};
//
class PulseTrigger {
 public:
  PulseTrigger() = default;
  ~PulseTrigger() = default;

  void Trigger();
  double GenNext();

  double amplitude_{0.3};
  int pulse_counter_{0};
  int pulse_length_{44};  // samples
};

const std::array<bool, kNumOscillators> kDefaultOscConfig{true, true, true,
                                                          true, true, true};
const float kSquareOscAmplitude = 0.4;

class SquareOscillatorBank {
 public:
  SquareOscillatorBank();
  ~SquareOscillatorBank() = default;

  double DoGenerate();

  void Start();
  void Stop();

  void SetAmplitude(double amp);
  bool IsNoteOn();

 private:
  std::vector<std::unique_ptr<QBLimitedOscillator>> oscillators_;
};

class DrumModule {
 public:
  virtual void NoteOn(double vel) = 0;
  virtual StereoVal Generate() = 0;

  double velocity_{1};

  EnvelopeGenerator eg_;
  DCA dca_;
  bool use_distortion_{false};
  Distortion distortion_;
  bool use_delay_{false};
  std::unique_ptr<StereoDelay> delay_;
};

const double kDefaultKickFrequency = 80;

class BassDrum : public DrumModule {
 public:
  BassDrum();
  virtual ~BassDrum() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  bool hard_sync_{false};
  double frequency_{kDefaultKickFrequency};

  PulseTrigger click_;

  bool noise_enabled_{false};
  std::unique_ptr<QBLimitedOscillator> noise_;
  EnvelopeGenerator noise_eg_;
  // std::unique_ptr<CKThreeFive> noise_filter_;
  std::unique_ptr<FilterSem> noise_filter_;

  std::unique_ptr<QBLimitedOscillator> osc1_;
  std::unique_ptr<QBLimitedOscillator> osc2_;

  std::unique_ptr<CKThreeFive> out_filter_;

  // DCA dca_;
};

const float kHighSnareFreq = 476;
const float kLowSnareFreq = 238;

class SnareDrum : public DrumModule {
 public:
  SnareDrum();
  virtual ~SnareDrum() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  float low_freq_{kLowSnareFreq};
  float high_freq_{kHighSnareFreq};

  std::unique_ptr<QBLimitedOscillator> noise_;
  EnvelopeGenerator noise_eg_;
  std::unique_ptr<CKThreeFive> noise_filter_;

  std::unique_ptr<QBLimitedOscillator> lo_osc_;
  std::unique_ptr<QBLimitedOscillator> hi_osc_;
};

class HandClap : public DrumModule {
 public:
  HandClap();
  virtual ~HandClap() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  std::unique_ptr<QBLimitedOscillator> noise_;
  EnvelopeGenerator noise_eg_;
  std::unique_ptr<FilterSem> noise_filter_;

  std::unique_ptr<LFO> lfo_;
};

class HiHat : public DrumModule {
 public:
  HiHat();
  virtual ~HiHat() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  void SetAmplitude(double val);

  SquareOscillatorBank osc_bank_;
  // std::unique_ptr<FilterSem> mid_filter_;
  std::unique_ptr<MoogLadder> mid_filter_;
  std::unique_ptr<FilterSem> high_filter_;
  // std::unique_ptr<MoogLadder> high_filter_;
};

class FMDrum : public DrumModule {
 public:
  FMDrum();
  virtual ~FMDrum() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  std::unique_ptr<QBLimitedOscillator> carrier_;

  std::unique_ptr<QBLimitedOscillator> modulator_;
  EnvelopeGenerator modulator_eg_;
  double modulator_freq_ratio_{2};
};

const double kDefaultPitchOscRange = 47;

class Lazer : public DrumModule {
 public:
  Lazer();
  virtual ~Lazer() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  std::unique_ptr<QBLimitedOscillator> osc1_;
  EnvelopeGenerator eg_;

  double pitch_osc_range_{kDefaultPitchOscRange};
};

}  // namespace SBAudio
