#pragma once

#include <fx/ddlmodule.h>
#include <fx/fx.h>
#include <wt_oscillator.h>

enum class ModDelayAlgorithm { kFlanger, kChorus, kVibrato };

class ModDelay : Fx {
 public:
  ModDelay();
  std::string Status() override;
  stereo_val Process(stereo_val input) override;
  void SetParam(std::string name, double val) override;
  double GetParam(std::string name) override;

 private:
  void Init();
  bool Update();
  void UpdateLfo();
  void UpdateDdl();
  void CookModType();
  double CalculateDelayOffset(double lfo_sample);
  bool PrepareForPlay();
  stereo_val ProcessAudio(stereo_val input);

  void SetDepth(double val);
  void SetRate(double val);
  void SetFeedbackPercent(double val);
  void SetChorusOffset(double val);
  void SetModType(unsigned int val);
  void SetLfoType(unsigned int val);

 private:
  WTOscillator m_lfo_;
  DDLModule m_ddl_;

  double m_min_delay_msec_{0};
  double m_max_delay_msec_{0};

  double m_mod_depth_pct_{0};
  double m_mod_freq_{0};
  double m_feedback_percent_{0};

  double m_chorus_offset_{0};

  ModDelayAlgorithm m_mod_type_{0};  // FLANGER, VIBRATO, CHORUS
  unsigned int m_lfo_type_{0};       // TRI / SINE
  unsigned int m_lfo_phase_{0};      // NORMAL / QUAD / INVERT
};
