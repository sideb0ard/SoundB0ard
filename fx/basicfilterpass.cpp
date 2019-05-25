#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "basicfilterpass.h"
#include "defjams.h"
#include "filter_moogladder.h"
#include "utils.h"

const char *filtertype_to_name[] = {"LPF1", "HPF1", "LPF2", "HPF2", "BPF2",
                                    "BSF2", "LPF4", "HPF4", "BPF4"};

filterpass *new_filterpass()
{
    filterpass *fp = (filterpass *)calloc(1, sizeof(filterpass));
    fp->m_fx.type = BASICFILTER;
    fp->m_fx.enabled = true;
    fp->m_fx.status = &filterpass_status;
    fp->m_fx.process = &filterpass_process_audio;
    fp->m_fx.event_notify = &fx_noop_event_notify;

    filter_moog_init(&fp->m_filter);
    fp->m_filter.f.m_aux_control = 0.0;

    osc_new_settings((oscillator *)&fp->m_lfo1);
    lfo_set_sound_generator_interface(&fp->m_lfo1);
    lfo_start_oscillator((oscillator *)&fp->m_lfo1);

    osc_new_settings((oscillator *)&fp->m_lfo2);
    lfo_set_sound_generator_interface(&fp->m_lfo2);
    lfo_start_oscillator((oscillator *)&fp->m_lfo2);

    printf("LFO1 freq is %.2f\n", fp->m_lfo1.osc.m_osc_fo);

    return fp;
}

void filterpass_status(void *self, char *status_string)
{
    filterpass *fp = (filterpass *)self;
    // clang-format off
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "freq:%.2f q:%.2f type:%s lfo1_active:%d lfo1_type:%d lfo1_amp:%.2f\n"
             "lfo1_rate:%.2f lfo2_active:%d lfo2_type:%d lfo2_amp:%.2f lfo2_rate:%.2f",
             fp->m_filter.f.m_fc_control, fp->m_filter.f.m_q_control,
             filtertype_to_name[fp->m_filter.f.m_filter_type],
             fp->m_lfo1_active, fp->m_lfo1.osc.m_waveform,
             fp->m_lfo1.osc.m_amplitude, fp->m_lfo1.osc.m_osc_fo,
             fp->m_lfo2_active, fp->m_lfo2.osc.m_waveform,
             fp->m_lfo2.osc.m_amplitude, fp->m_lfo2.osc.m_osc_fo);
    // clang-format on
}

stereo_val filterpass_process_audio(void *self, stereo_val input)
{
    filterpass *fp = (filterpass *)self;

    double lfo1_val = 0.0;
    double lfo2_val = 0.0;

    if (fp->m_lfo1_active)
    {
        osc_update((oscillator *)&fp->m_lfo1);
        lfo1_val = lfo_do_oscillate((oscillator *)&fp->m_lfo1, NULL);
        filter_set_fc_mod((filter *)&fp->m_filter,
                          lfo1_val * FILTER_FC_MOD_RANGE);
    }

    if (fp->m_lfo2_active)
    {
        osc_update((oscillator *)&fp->m_lfo2);
        lfo2_val = lfo_do_oscillate((oscillator *)&fp->m_lfo2, NULL);
        moog_set_qcontrol((filter *)&fp->m_filter,
                          bipolar_to_unipolar(lfo2_val));
    }

    moog_update((filter *)&fp->m_filter);
    input.left = moog_gennext((filter *)&fp->m_filter, input.left);
    input.right = moog_gennext((filter *)&fp->m_filter, input.right);

    return input;
}

void filterpass_set_lfo_active(filterpass *fp, int lfo_num, bool b)
{
    switch (lfo_num)
    {
    case (1):
        fp->m_lfo1_active = b;
        break;
    case (2):
        fp->m_lfo2_active = b;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}

void filterpass_set_lfo_rate(filterpass *fp, int lfo_num, double val)
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
        fp->m_lfo1.osc.m_osc_fo = val;
        break;
    case (2):
        fp->m_lfo2.osc.m_osc_fo = val;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}

void filterpass_set_lfo_amp(filterpass *fp, int lfo_num, double val)
{
    if (val < 0. || val > 1.)
    {
        printf("Val out of range - must be between 0 and 1\n");
        return;
    }
    switch (lfo_num)
    {
    case (1):
        fp->m_lfo1.osc.m_amplitude = val;
        break;
    case (2):
        fp->m_lfo2.osc.m_amplitude = val;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}

void filterpass_set_lfo_type(filterpass *fp, int lfo_num, unsigned int type)
{
    if (type >= MAX_LFO_OSC)
    {
        printf("Val out of range - must be < %d\n", MAX_LFO_OSC);
        return;
    }
    switch (lfo_num)
    {
    case (1):
        fp->m_lfo1.osc.m_waveform = type;
        break;
    case (2):
        fp->m_lfo2.osc.m_waveform = type;
        break;
    default:
        printf("Only got two LFO's mate - what do you think i am?\n");
    }
}
