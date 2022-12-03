#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <envelope_generator.h>
#include <fx/fx.h>

enum { ENV_MODE_TRIGGER, ENV_MODE_SUSTAIN };

class Envelope : Fx {
 public:
  Envelope();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void EventNotify(broadcast_event event, mixer_timing_info tinfo) override;
  void SetParam(std::string name, double val) override;

 private:
  void Reset();
  void CalculateTimings();
  void SetLengthBars(double length_bars);
  void SetAttackMs(double val);
  void SetDecayMs(double val);
  void SetSustainLvl(double val);
  void SetReleaseMs(double val);
  void SetType(unsigned int type);  // analog or digital
  void SetMode(unsigned int mode);  // sustain or trigger
  void SetDebug(bool b);

 private:
  EnvelopeGenerator eg_;
  bool started_;

  unsigned int env_mode_;  // trigger or sustain
  double env_length_bars_;
  int env_length_ticks_;
  int env_length_ticks_counter_;
  int release_tick_;

  unsigned int eg_state_;
  bool debug_;
};

#endif
