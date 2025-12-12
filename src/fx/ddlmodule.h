#pragma once
#include <defjams.h>

#include <vector>

class DDLModule {
 public:
  DDLModule();
  DDLModule(double max_delay_ms);
  ~DDLModule() = default;

  std::vector<double> m_buffer;

  double m_delay_in_samples{0};
  double m_feedback{0};
  double m_wet_level{0};
  // TODO - attentuation??

  int m_read_index{0};
  int m_write_index{0};

  bool m_use_external_feedback{false};
  double m_feedback_in{0};

  // User Controls
  double m_delay_ms{0};
  double m_feedback_pct{0};
  double m_wet_level_pct{0};

 public:
  bool ProcessAudio(double *input, double *output);
  void Update();
  void ResetDelay();
  void SetDelayMs(double delay_time_ms);
  double ReadDelay();
  double ReadDelayAt(double time_in_ms);
  void WriteDelayAndInc(double val);
};
