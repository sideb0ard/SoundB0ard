#pragma once

#include <stdbool.h>

#include <defjams.h>
#include <fx/delayline.h>
#include <fx/fx.h>
#include <lfo.h>

typedef enum
{
    TAP1,
    TAP2,
    PINGPONG,
    MAX_NUM_DELAY_MODE
} delay_mode;

typedef enum
{
    DELAY_SYNC_QUARTER,
    DELAY_SYNC_EIGHTH,
    DELAY_SYNC_SIXTEENTH,
    DELAY_SYNC_SIZE,
} delay_sync_len;

class StereoDelay : Fx
{

  public:
    StereoDelay(double duration);
    ~StereoDelay() = default;
    void Status(char *string) override;
    stereo_val Process(stereo_val input) override;
    void EventNotify(broadcast_event event) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  private:
    void SetMode(unsigned mode);
    void SetDelayTimeMs(double delay_ms);
    void SetFeedbackPercent(double feedback_percent);
    void SetDelayRatio(double delay_ratio);
    void SetWetMix(double wet_mix);

    void SetSync(bool b);
    void SetSyncLen(unsigned int);
    void SyncTempo();

    void PrepareForPlay();
    void Reset();
    void Update();
    bool ProcessAudio(double *input_left, double *input_right,
                      double *output_left, double *output_right);

  private:
    delayline m_left_delay_;
    delayline m_right_delay_;
    double m_delay_time_ms_;    // 0 - 2000
    double m_feedback_percent_; // -100 - 100
    double m_delay_ratio_;
    double m_wet_mix_; // 0 - 100
    unsigned m_mode_;
    double m_tap2_left_delay_time_ms_;
    double m_tap2_right_delay_time_ms_;

    lfo m_lfo1_;
    bool lfo1_on_;
    double m_lfo1_min_;
    double m_lfo1_max_;

    bool sync_;
    unsigned int sync_len_;

    lfo m_lfo2_;
    bool lfo2_on_;
    double m_lfo2_min_;
    double m_lfo2_max_;
};
