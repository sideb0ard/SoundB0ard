#pragma once

#include <dca.h>
#include <defjams.h>
#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <fx/envelope.h>
#include <fx/fx.h>
#include <qblimited_oscillator.h>
#include <soundgenerator.h>
#include <stdbool.h>

#include <atomic>

namespace SBAudio {
class SBSynth : public SoundGenerator {
 public:
  SBSynth();
  virtual ~SBSynth() = default;

  stereo_val GenNext(mixer_timing_info tinfo) override;

  std::string Info() override;
  std::string Status() override;

  void SetParam(std::string name, double val) override;

  void start() override;
  void stop() override;

  void noteOn(midi_event ev) override;
  void noteOff(midi_event ev) override;

 private:
  QBLimitedOscillator m_osc1;
  QBLimitedOscillator m_osc2;

  float m_osc1_amp{1};
  float m_osc2_amp{1};

  float cm_ratio{0.5};

  EnvelopeGenerator m_eg1;
  DCA m_dca;
};

};  // namespace SBAudio
