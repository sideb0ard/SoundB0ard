#pragma once

#include <fx/ddlmodule.h>
#include <fx/fx.h>
#include <wt_oscillator.h>

typedef enum
{
    FLANGER,
    VIBRATO,
    CHORUS,
    MAX_MOD_TYPE
} modular_type;

class ModDelay : Fx
{
  public:
    ModDelay();
    void Status(char *string) override;
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
    bool ProcessAudio(double *input_left, double *input_right,
                      double *output_left, double *output_right);

    void SetDepth(double val);
    void SetRate(double val);
    void SetFeedbackPercent(double val);
    void SetChorusOffset(double val);
    void SetModType(unsigned int val);
    void SetLfoType(unsigned int val);

  private:
    WTOscillator m_lfo_;
    ddlmodule m_ddl_;

    double m_min_delay_msec_;
    double m_max_delay_msec_;

    double m_mod_depth_pct_;
    double m_mod_freq_;
    double m_feedback_percent_;
    double m_chorus_offset_;

    unsigned int m_mod_type_;  // FLANGER, VIBRATO, CHORUS
    unsigned int m_lfo_type_;  // TRI / SINE
    unsigned int m_lfo_phase_; // NORMAL / QUAD / INVERT
};
