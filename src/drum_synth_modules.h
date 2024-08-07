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
  Distortion distortion_;
  std::unique_ptr<StereoDelay> delay_;
};

const double kDefaultKickFrequency = 48.9994;  // G1

class BassDrum : public DrumModule {
 public:
  BassDrum();
  virtual ~BassDrum() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  bool hard_sync_{false};
  double frequency_{kDefaultKickFrequency};

  std::unique_ptr<QBLimitedOscillator> noise_;
  EnvelopeGenerator noise_eg_;
  std::unique_ptr<CKThreeFive> noise_filter_;

  std::unique_ptr<QBLimitedOscillator> osc1_;
  std::unique_ptr<QBLimitedOscillator> osc2_;

  std::unique_ptr<CKThreeFive> out_filter_;

  DCA dca_;
};

const float kHighSnareFreq = 476;
const float kLowSnareFreq = 238;

class SnareDrum : public DrumModule {
 public:
  SnareDrum();
  virtual ~SnareDrum() = default;

  void NoteOn(double vel) override;
  StereoVal Generate() override;

  std::unique_ptr<QBLimitedOscillator> noise_;
  EnvelopeGenerator noise_eg_;
  std::unique_ptr<CKThreeFive> noise_filter_;

  std::unique_ptr<QBLimitedOscillator> osc1_;
  std::unique_ptr<QBLimitedOscillator> osc2_;
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

}  // namespace SBAudio
