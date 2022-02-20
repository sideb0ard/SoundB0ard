#pragma once

#include <defjams.h>

#include "delay.h"
#include "delayapf.h"
#include "envelope_detector.h"
#include "fx.h"
#include "lpfcombfilter.h"
#include "onepolelpf.h"

enum { COMP, LIMIT, EXPAND, GATE };

class DynamicsProcessor : Fx {
 public:
  DynamicsProcessor();
  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;

  void SetExternalSource(unsigned int val);
  void SetDefaultSidechainParams();

 private:
  void Init();

  double CalcCompressionGain(double detector_val, double threshold,
                             double rratio, double kneewidth, bool limit);

  double CalcDownwardExpanderGain(double detector_val, double threshold,
                                  double rratio, double kneewidth, bool gate);

  void SetInputGainDb(double val);
  void SetThreshold(double val);
  void SetAttackMs(double val);
  void SetReleaseMs(double val);
  void SetRatio(double val);
  void SetOutputGainDb(double val);
  void SetKneeWidth(double val);
  void SetLookaheadDelayMs(double val);
  void SetStereoLink(unsigned int val);
  void SetProcessorType(unsigned int val);
  void SetTimeConstant(unsigned int val);

 private:
  envelope_detector m_left_detector_;
  envelope_detector m_right_detector_;

  delay m_left_delay_;
  delay m_right_delay_;

  double m_inputgain_db_;
  double m_threshold_;
  double m_attack_ms_;
  double m_release_ms_;
  double m_ratio_;
  double m_outputgain_db_;
  double m_knee_width_;
  double m_lookahead_delay_ms_;
  unsigned int m_stereo_link_;     // on, off
  unsigned int m_processor_type_;  // comp, limit, expand, gate
  unsigned int m_time_constant_;   // digital, analog
  int m_external_source_;  // a sound_generator id that will correspond to
                           // mixer input cache
};
