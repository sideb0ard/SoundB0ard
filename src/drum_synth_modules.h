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
#include "lfo.h"
#include "qblimited_oscillator.h"

namespace SBAudio {

const int kNumOscillators{6};
const std::array<float, kNumOscillators> kOscFrequencies{263, 400, 421,
                                                         474, 587, 845};
const std::array<bool, kNumOscillators> kDefaultOscConfig{true, true, true,
                                                          true, true, true};

class DrumModule {
 public:
  virtual void NoteOn(double amp) = 0;
  virtual StereoVal Generate() = 0;
};

const double kDefaultKickFrequency = 48.9994;  // G1

class BassDrum : public DrumModule {
 public:
  BassDrum();
  virtual ~BassDrum() = default;

  void NoteOn(double amp) override;
  StereoVal Generate() override;

 private:
  bool hard_sync_{false};
  double detune_cents_{0};
  double frequency_{kDefaultKickFrequency};

  std::unique_ptr<QBLimitedOscillator> noise_;
  EnvelopeGenerator noise_eg_;
  std::unique_ptr<CKThreeFive> noise_filter_;

  std::unique_ptr<QBLimitedOscillator> osc1_;
  std::unique_ptr<QBLimitedOscillator> osc2_;

  // used for Oscillator pitch and DCA amplitude
  EnvelopeGenerator eg_;

  Distortion distortion_;

  std::unique_ptr<CKThreeFive> out_filter_;

  DCA out_;
};

class SquareOscillatorBank {
 public:
  SquareOscillatorBank();
  ~SquareOscillatorBank() = default;

  double DoOutput();

  void Start();
  void Stop();

 private:
  std::vector<std::unique_ptr<QBLimitedOscillator>> oscillators_;
};

}  // namespace SBAudio
