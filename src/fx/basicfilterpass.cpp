#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <defjams.h>
#include <filter_moogladder.h>
#include <fx/basicfilterpass.h>
#include <utils.h>

const char *filtertype_to_name[] = {"LPF1", "HPF1", "LPF2", "HPF2", "BPF2",
                                    "BSF2", "LPF4", "HPF4", "BPF4"};

FilterPass ::FilterPass()
{
    type_ = BASICFILTER;
    enabled_ = true;

    filter_moog_init(&m_filter_);
    m_filter_.f.m_aux_control = 0.0;

    osc_new_settings((oscillator *)&m_lfo1_);
    lfo_set_sound_generator_interface(&m_lfo1_);
    lfo_start_oscillator((oscillator *)&m_lfo1_);

    osc_new_settings((oscillator *)&m_lfo2_);
    lfo_set_sound_generator_interface(&m_lfo2_);
    lfo_start_oscillator((oscillator *)&m_lfo2_);

    printf("LFO1 freq is %.2f\n", m_lfo1_.osc.m_osc_fo);
}

void FilterPass::Status(char *status_string)
{
    // clang-format off
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "freq:%.2f q:%.2f type:%s lfo1_active:%d lfo1_type:%d lfo1_amp:%.2f\n"
             "lfo1_rate:%.2f lfo2_active:%d lfo2_type:%d lfo2_amp:%.2f lfo2_rate:%.2f",
             m_filter_.f.m_fc_control, m_filter_.f.m_q_control,
             filtertype_to_name[m_filter_.f.m_filter_type],
             m_lfo1_active_, m_lfo1_.osc.m_waveform,
             m_lfo1_.osc.m_amplitude, m_lfo1_.osc.m_osc_fo,
             m_lfo2_active_, m_lfo2_.osc.m_waveform,
             m_lfo2_.osc.m_amplitude, m_lfo2_.osc.m_osc_fo);
    // clang-format on
}

stereo_val FilterPass::Process(stereo_val input)
{
    double lfo1_val = 0.0;
    double lfo2_val = 0.0;

    if (m_lfo1_active_)
    {
        osc_update((oscillator *)&m_lfo1_);
        lfo1_val = lfo_do_oscillate((oscillator *)&m_lfo1_, NULL);
        filter_set_fc_mod((filter *)&m_filter_, lfo1_val * FILTER_FC_MOD_RANGE);
    }

    if (m_lfo2_active_)
    {
        osc_update((oscillator *)&m_lfo2_);
        lfo2_val = lfo_do_oscillate((oscillator *)&m_lfo2_, NULL);
        moog_set_qcontrol((filter *)&m_filter_, bipolar_to_unipolar(lfo2_val));
    }

    moog_update((filter *)&m_filter_);
    input.left = moog_gennext((filter *)&m_filter_, input.left);
    input.right = moog_gennext((filter *)&m_filter_, input.right);

    return input;
}

void FilterPass::SetParam(std::string name, double val) {}
double FilterPass::GetParam(std::string name) { return 0; }

void FilterPass::SetLfoType(int lfo_num, unsigned int type)
{
    if (type >= MAX_LFO_OSC)
    {
        printf("Val out of range - must be < %d\n", MAX_LFO_OSC);
        return;
    }
    switch (lfo_num)
    {
    case (1):
        m_lfo1_.osc.m_waveform = type;
        break;
    case (2):
        m_lfo2_.osc.m_waveform = type;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}

void FilterPass::SetLfoActive(int lfo_num, bool b)
{
    switch (lfo_num)
    {
    case (1):
        m_lfo1_active_ = b;
        break;
    case (2):
        m_lfo2_active_ = b;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}

void FilterPass::SetLfoRate(int lfo_num, double val)
{
    if (val < MIN_LFO_RATE || val > MAX_LFO_RATE)
    {
        printf("Val out of range - must be between %f and %f\n", MIN_LFO_RATE,
               MAX_LFO_RATE);
        return;
    }
    switch (lfo_num)
    {
    case (1):
        m_lfo1_.osc.m_osc_fo = val;
        break;
    case (2):
        m_lfo2_.osc.m_osc_fo = val;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}

void FilterPass::SetLfoAmp(int lfo_num, double val)
{
    if (val < 0. || val > 1.)
    {
        printf("Val out of range - must be between 0 and 1\n");
        return;
    }
    switch (lfo_num)
    {
    case (1):
        m_lfo1_.osc.m_amplitude = val;
        break;
    case (2):
        m_lfo2_.osc.m_amplitude = val;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}
