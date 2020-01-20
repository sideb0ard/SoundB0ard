#include <mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <fx/stereodelay.h>

extern mixer *mixr;

StereoDelay::StereoDelay(double duration) : m_delay_time_ms_{duration}
{
    m_feedback_percent_ = 2;
    m_delay_ratio_ = 0.2;
    m_wet_mix_ = 0.7;
    m_mode_ = TAP2;

    type_ = DELAY;
    enabled_ = true;

    sync_ = true;
    sync_len_ = DELAY_SYNC_SIXTEENTH;

    lfo1_on_ = false;
    m_lfo1_min_ = 50;
    m_lfo1_max_ = 80;
    osc_new_settings((oscillator *)&m_lfo1_);
    lfo_set_sound_generator_interface(&m_lfo1_);
    m_lfo1_.osc.m_osc_fo = 0.01; // default LFO
    m_lfo1_.osc.m_amplitude = 1.;
    lfo_start_oscillator((oscillator *)&m_lfo1_);

    lfo2_on_ = false;
    m_lfo2_min_ = 50;
    m_lfo2_max_ = 80;
    osc_new_settings((oscillator *)&m_lfo2_);
    lfo_set_sound_generator_interface(&m_lfo2_);
    m_lfo2_.osc.m_osc_fo = 0.01; // default LFO
    m_lfo2_.osc.m_amplitude = 1.;
    lfo_start_oscillator((oscillator *)&m_lfo2_);

    PrepareForPlay();
    Update();

    SyncTempo();
}

void StereoDelay::EventNotify(broadcast_event event)
{
    if (event.type == TIME_BPM_CHANGE)
    {
        if (sync_)
            SyncTempo();
    }
}

void StereoDelay::Reset()
{
    delayline_reset(&m_left_delay_);
    delayline_reset(&m_right_delay_);
}

void StereoDelay::PrepareForPlay()
{
    // delay line memory allocated here
    delayline_init(&m_left_delay_, 2.0 * SAMPLE_RATE);
    delayline_init(&m_right_delay_, 2.0 * SAMPLE_RATE);
    Reset();
}

void StereoDelay::SetMode(unsigned mode)
{
    if (mode < MAX_NUM_DELAY_MODE)
        m_mode_ = mode;
    else
        printf("Delay mode must be between 0 and %d\n", MAX_NUM_DELAY_MODE);
}

void StereoDelay::SetDelayTimeMs(double delay_ms)
{
    if (delay_ms >= 0 && delay_ms <= 2000)
    {
        m_delay_time_ms_ = delay_ms;
        Update();
    }
    else
        printf("Delay time ms must be between 0 and 2000\n");
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
    if (delay_ratio >= -1 && delay_ratio <= 1)
    {
        m_delay_ratio_ = delay_ratio;
        Update();
    }
    else
        printf("Delay ratio must be between -1 and 1\n");
}

void StereoDelay::SetWetMix(double wet_mix)
{
    if (wet_mix >= 0 && wet_mix <= 100)
    {
        m_wet_mix_ = wet_mix;
        Update();
    }
    else
        printf("Wetmix must be between 0 and 100\n");
}

void StereoDelay::Update()
{
    if (m_mode_ == TAP1 || m_mode_ == TAP2)
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
        delayline_set_delay_ms(&m_left_delay_, m_delay_time_ms_);
        delayline_set_delay_ms(&m_right_delay_, m_delay_time_ms_);

        return;
    }

    // else
    m_tap2_left_delay_time_ms_ = 0.0;
    m_tap2_right_delay_time_ms_ = 0.0;

    if (m_delay_ratio_ < 0)
    {
        delayline_set_delay_ms(&m_left_delay_,
                               -m_delay_ratio_ * m_delay_time_ms_);
        delayline_set_delay_ms(&m_right_delay_, m_delay_time_ms_);
    }
    else if (m_delay_ratio_ > 0)
    {
        delayline_set_delay_ms(&m_left_delay_, m_delay_time_ms_);
        delayline_set_delay_ms(&m_right_delay_,
                               m_delay_ratio_ * m_delay_time_ms_);
    }
    else
    {
        delayline_set_delay_ms(&m_left_delay_, m_delay_time_ms_);
        delayline_set_delay_ms(&m_right_delay_, m_delay_time_ms_);
    }
}

bool StereoDelay::ProcessAudio(double *input_left, double *input_right,
                               double *output_left, double *output_right)
{
    double left_delay_out = delayline_read_delay(&m_left_delay_);
    double right_delay_out = delayline_read_delay(&m_right_delay_);

    double left_delay_in =
        *input_left + left_delay_out * (m_feedback_percent_ / 100.0);
    double right_delay_in =
        *input_right + right_delay_out * (m_feedback_percent_ / 100.0);

    double left_tap2_out = 0.0;
    double right_tap2_out = 0.0;

    switch (m_mode_)
    {
    case TAP1:
    {
        left_tap2_out =
            delayline_read_delay_at(&m_left_delay_, m_tap2_left_delay_time_ms_);
        right_tap2_out = delayline_read_delay_at(&m_right_delay_,
                                                 m_tap2_right_delay_time_ms_);
        break;
    }
    case TAP2:
    {
        left_tap2_out =
            delayline_read_delay_at(&m_left_delay_, m_tap2_left_delay_time_ms_);
        right_tap2_out = delayline_read_delay_at(&m_right_delay_,
                                                 m_tap2_right_delay_time_ms_);
        left_delay_in =
            *input_left + (0.5 * left_delay_out + 0.5 * left_tap2_out) *
                              (m_feedback_percent_ / 100.0);
        right_delay_in =
            *input_right + (0.5 * right_delay_out + 0.5 * right_tap2_out) *
                               (m_feedback_percent_ / 100.0);
        break;
    }
    case PINGPONG:
    {
        left_delay_in =
            *input_right + right_delay_out * (m_feedback_percent_ / 100.0);
        right_delay_in =
            *input_left + left_delay_out * (m_feedback_percent_ / 100.0);
        break;
    }
    }

    double left_out = 0.0;
    double right_out = 0.0;

    delayline_process_audio(&m_left_delay_, &left_delay_in, &left_out);
    delayline_process_audio(&m_right_delay_, &right_delay_in, &right_out);

    *output_left = *input_left * (1.0 - m_wet_mix_) +
                   m_wet_mix_ * (left_out + left_tap2_out);
    *output_right = *input_right * (1.0 - m_wet_mix_) +
                    m_wet_mix_ * (right_out + right_tap2_out);

    return true;
}

constexpr char const *s_delay_mode[] = {"TAP1", "TAP2", "PINGPONG"};
constexpr char const *s_delay_sync_len[] = {"4TH", "8TH", "16TH"};

void StereoDelay::Status(char *status_string)
{
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "delayms:%.0f fb:%.2f ratio:%.2f "
             "wetmx:%.2f mode:%s(%d) sync:%d synclen:%s(%d)",
             m_delay_time_ms_, m_feedback_percent_, m_delay_ratio_, m_wet_mix_,
             s_delay_mode[m_mode_], m_mode_, sync_, s_delay_sync_len[sync_len_],
             sync_len_);
}

stereo_val StereoDelay::Process(stereo_val input)
{
    stereo_val output = {};
    ProcessAudio(&input.left, &input.right, &output.left, &output.right);
    return output;
}

void StereoDelay::SyncTempo()
{
    double delay_time_quarter_note_ms = 60 / mixr->bpm * 1000;
    if (sync_len_ == DELAY_SYNC_QUARTER)
        m_delay_time_ms_ = delay_time_quarter_note_ms;
    else if (sync_len_ == DELAY_SYNC_EIGHTH)
        m_delay_time_ms_ = delay_time_quarter_note_ms * 0.5;
    else if (sync_len_ == DELAY_SYNC_SIXTEENTH)
        m_delay_time_ms_ = delay_time_quarter_note_ms * 0.25;

    Update();
}

void StereoDelay::SetSync(bool b)
{
    sync_ = b;
    if (b)
        SyncTempo();
}

void StereoDelay::SetSyncLen(unsigned int len)
{
    if (len < DELAY_SYNC_SIZE)
    {
        sync_len_ = len;
        if (sync_)
            SyncTempo();
    }
}

void StereoDelay::SetParam(std::string name, double val) {}
double StereoDelay::GetParam(std::string name) { return 0; }
