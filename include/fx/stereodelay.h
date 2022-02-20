#pragma once

#include <defjams.h>
#include <fx/ddlmodule.h>
#include <fx/fx.h>
#include <lfo.h>
#include <stdbool.h>

enum DelayMode {

  norm,
  tap1,
  tap2,
  pingpong,
};

enum DelaySyncLen {
  NO_DELAY,
  QUARTER,
  EIGHTH,
  SIXTEENTH,
};

class StereoDelay : Fx {
 public:
  StereoDelay();
  ~StereoDelay() = default;
  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;

  void SetMode(unsigned mode);
  void SetDelayTimeMs(double delay_ms);
  void SetFeedbackPercent(double feedback_percent);
  void SetDelayRatio(double delay_ratio);
  void SetWetMix(double wet_mix);

  void SyncTempo();
  void SetSync(bool b);
  void SetSyncLen(unsigned int);

  void Reset();
  void Update();
  bool ProcessAudio(double *input_left, double *input_right,
                    double *output_left, double *output_right);

 private:
  DDLModule m_left_delay_;
  DDLModule m_right_delay_;
  double m_delay_time_ms_{0};     // 0 - 2000
  double m_feedback_percent_{0};  // -100 - 100
  double m_delay_ratio_{0};
  double m_wet_mix_{0.5};  // 0 - 100
  double m_tap2_left_delay_time_ms_{0};
  double m_tap2_right_delay_time_ms_{0};
  DelayMode m_mode_{DelayMode::norm};
  DelaySyncLen sync_len_{DelaySyncLen::SIXTEENTH};
  bool sync_{false};
};
