#include <mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <iostream>

#include <fx/stereodelay.h>

extern Mixer *mixr;

StereoDelay::StereoDelay() { enabled_ = true; }

void StereoDelay::Reset()
{
    m_left_delay_.ResetDelay();
    m_right_delay_.ResetDelay();
}

void StereoDelay::SetDelayTimeMs(double delay_ms)
{
    if (delay_ms >= 0 && delay_ms <= kMaxDelayLenSecs * 1000)
    {
        m_delay_time_ms_ = delay_ms;
        Update();
    }
    else
        std::cerr << "Delay time ms must be between 0 and "
                  << kMaxDelayLenSecs * 1000 << std::endl;
}

void StereoDelay::SetFeedbackPercent(double feedback_percent)
{
    if (feedback_percent >= -100 && feedback_percent <= 100)
    {
        m_feedback_percent_ = feedback_percent;
        Update();
    }
    else
        printf("Feedback %% must be between -100 and 100\n");
}

void StereoDelay::SetDelayRatio(double delay_ratio)
{
    if (delay_ratio > -1 && delay_ratio < 1)
    {
        m_delay_ratio_ = delay_ratio;
        Update();
    }
    else
        printf("Delay ratio must be between -1 and 1\n");
}

void StereoDelay::SetWetMix(double wet_mix)
{
    if (wet_mix >= 0 && wet_mix <= 1)
    {
        m_wet_mix_ = wet_mix;
        Update();
    }
    else
        printf("Wetmix must be between 0 and 100\n");
}

void StereoDelay::Update()
{
    if (m_mode_ == DelayMode::tap1 || m_mode_ == DelayMode::tap2)
    {
        if (m_delay_ratio_ < 0)
        {
            m_tap2_left_delay_time_ms_ = -m_delay_ratio_ * m_delay_time_ms_;
            m_tap2_right_delay_time_ms_ =
                (1.0 + m_delay_ratio_) * m_delay_time_ms_;
        }
        else if (m_delay_ratio_ > 0)
        {
            m_tap2_left_delay_time_ms_ =
                (1.0 - m_delay_ratio_) * m_delay_time_ms_;
            m_tap2_right_delay_time_ms_ = m_delay_ratio_ * m_delay_time_ms_;
        }
        else
        {
            m_tap2_left_delay_time_ms_ = 0.0;
            m_tap2_right_delay_time_ms_ = 0.0;
        }
        m_left_delay_.SetDelayMs(m_delay_time_ms_);
        m_right_delay_.SetDelayMs(m_delay_time_ms_);

        return;
    }

    // else
    m_tap2_left_delay_time_ms_ = 0.0;
    m_tap2_right_delay_time_ms_ = 0.0;

    if (m_delay_ratio_ < 0)
    {
        m_left_delay_.SetDelayMs(-m_delay_ratio_ * m_delay_time_ms_);
        m_right_delay_.SetDelayMs(m_delay_time_ms_);
    }
    else if (m_delay_ratio_ > 0)
    {
        m_left_delay_.SetDelayMs(m_delay_time_ms_);
        m_right_delay_.SetDelayMs(m_delay_ratio_ * m_delay_time_ms_);
    }
    else
    {
        m_left_delay_.SetDelayMs(m_delay_time_ms_);
        m_right_delay_.SetDelayMs(m_delay_time_ms_);
    }
}

bool StereoDelay::ProcessAudio(double *input_left, double *input_right,
                               double *output_left, double *output_right)
{
    double left_delay_out = m_left_delay_.ReadDelay();
    double right_delay_out = m_right_delay_.ReadDelay();

    double left_delay_in =
        *input_left + left_delay_out * (m_feedback_percent_ / 100.0);
    double right_delay_in =
        *input_right + right_delay_out * (m_feedback_percent_ / 100.0);

    double left_tap2_out = 0.0;
    double right_tap2_out = 0.0;

    switch (m_mode_)
    {
    case DelayMode::tap1:
    {
        left_tap2_out = m_left_delay_.ReadDelayAt(m_tap2_left_delay_time_ms_);
        right_tap2_out =
            m_right_delay_.ReadDelayAt(m_tap2_right_delay_time_ms_);
        break;
    }
    case DelayMode::tap2:
    {
        left_tap2_out = m_left_delay_.ReadDelayAt(m_tap2_left_delay_time_ms_);
        right_tap2_out =
            m_right_delay_.ReadDelayAt(m_tap2_right_delay_time_ms_);
        left_delay_in =
            *input_left + (0.5 * left_delay_out + 0.5 * left_tap2_out) *
                              (m_feedback_percent_ / 100.0);
        right_delay_in =
            *input_right + (0.5 * right_delay_out + 0.5 * right_tap2_out) *
                               (m_feedback_percent_ / 100.0);
        break;
    }
    case DelayMode::pingpong:
    {
        left_delay_in =
            *input_right + right_delay_out * (m_feedback_percent_ / 100.0);
        right_delay_in =
            *input_left + left_delay_out * (m_feedback_percent_ / 100.0);
        break;
    }
    default:
    {
    }
    }

    double left_out = 0.0;
    double right_out = 0.0;

    m_left_delay_.ProcessAudio(&left_delay_in, &left_out);
    m_right_delay_.ProcessAudio(&right_delay_in, &right_out);

    *output_left = *input_left * (1.0 - m_wet_mix_) +
                   m_wet_mix_ * (left_out + left_tap2_out);
    *output_right = *input_right * (1.0 - m_wet_mix_) +
                    m_wet_mix_ * (right_out + right_tap2_out);

    return true;
}

void StereoDelay::Status(char *status_string)
{
    std::string mode{};
    switch (m_mode_)
    {
    case DelayMode::norm:
        mode = "norm";
        break;
    case DelayMode::tap1:
        mode = "tap1";
        break;
    case DelayMode::tap2:
        mode = "tap2";
        break;
    case DelayMode::pingpong:
        mode = "pingpong";
    }
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "delayms:%.0f fb:%.2f ratio:%.2f "
             "wetmx:%.2f mode:%s ",
             m_delay_time_ms_, m_feedback_percent_, m_delay_ratio_, m_wet_mix_,
             mode.c_str());
}

stereo_val StereoDelay::Process(stereo_val input)
{
    stereo_val output = {};
    ProcessAudio(&input.left, &input.right, &output.left, &output.right);
    return output;
}

double StereoDelay::GetParam(std::string name) { return 0; }
void StereoDelay::SetParam(std::string name, double val)
{
    if (name == "delayms")
        SetDelayTimeMs(val);
    else if (name == "fb")
        SetFeedbackPercent(val);
    else if (name == "ratio")
        SetDelayRatio(val);
    else if (name == "wetmx")
        SetWetMix(val);
    else if (name == "mode")
        SetMode(val);
}
void StereoDelay::SetMode(unsigned mode)
{
    switch (mode)
    {
    case (0):
        m_mode_ = DelayMode::norm;
        break;
    case (1):
        m_mode_ = DelayMode::tap1;
        break;
    case (2):
        m_mode_ = DelayMode::tap2;
        break;
    case (3):
        m_mode_ = DelayMode::pingpong;
        break;
    }
}
