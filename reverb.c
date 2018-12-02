#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "reverb.h"

reverb *new_reverb(void)
{
    reverb *r = (reverb *)calloc(1, sizeof(reverb));

    r->m_pre_delay_msec = 40;
    r->m_pre_delay_atten_db = 0;
    r->m_input_lpf_g = 0.45;

    r->m_apf_1_delay_msec = 13.28;
    r->m_apf_1_g = 0.7;

    r->m_apf_2_delay_msec = 28.13;
    r->m_apf_2_g = -0.7;

    r->m_comb_1_delay_msec = 31.71;
    r->m_comb_2_delay_msec = 37.11;
    r->m_comb_3_delay_msec = 40.23;
    r->m_comb_4_delay_msec = 44.14;
    r->m_comb_5_delay_msec = 30.47;
    r->m_comb_6_delay_msec = 33.98;
    r->m_comb_7_delay_msec = 41.41;
    r->m_comb_8_delay_msec = 42.58;

    r->m_apf_3_delay_msec = 9.38;
    r->m_apf_3_g = -0.6;

    r->m_apf_4_delay_msec = 11;
    r->m_apf_4_g = 0.6;

    r->m_lpf2_g2 = 0.25;
    r->m_rt60 = 100;
    r->m_wet_pct = 20;

    reverb_init_reverb(r);

    r->m_fx.type = REVERB;
    r->m_fx.enabled = true;
    r->m_fx.status = &reverb_status;
    r->m_fx.process = &reverb_process_audio;
    r->m_fx.event_notify = &fx_noop_event_notify;

    return r;
}

void reverb_init_reverb(reverb *r)
{
    delay_init(&r->m_pre_delay, 2.0 * SAMPLE_RATE);

    delay_apf_init(&r->m_input_apf_1, 0.1 * SAMPLE_RATE);
    delay_apf_init(&r->m_input_apf_2, 0.1 * SAMPLE_RATE);

    comb_filter_init(&r->m_parallel_cf_1, 0.1 * SAMPLE_RATE);
    comb_filter_init(&r->m_parallel_cf_2, 0.1 * SAMPLE_RATE);
    lpf_comb_filter_init(&r->m_parallel_cf_3, 0.1 * SAMPLE_RATE);
    lpf_comb_filter_init(&r->m_parallel_cf_4, 0.1 * SAMPLE_RATE);
    comb_filter_init(&r->m_parallel_cf_5, 0.1 * SAMPLE_RATE);
    comb_filter_init(&r->m_parallel_cf_6, 0.1 * SAMPLE_RATE);
    lpf_comb_filter_init(&r->m_parallel_cf_7, 0.1 * SAMPLE_RATE);
    lpf_comb_filter_init(&r->m_parallel_cf_8, 0.1 * SAMPLE_RATE);

    delay_apf_init(&r->m_output_apf_3, 0.1 * SAMPLE_RATE);
    delay_apf_init(&r->m_output_apf_4, 0.1 * SAMPLE_RATE);

    one_pole_lpf_init(&r->m_input_lpf);
    one_pole_lpf_init(&r->m_damping_lpf_1);
    one_pole_lpf_init(&r->m_damping_lpf_2);

    delay_reset_delay(&r->m_pre_delay);
    delay_reset_delay(&r->m_input_apf_1.m_delay);
    delay_reset_delay(&r->m_input_apf_2.m_delay);

    delay_reset_delay(&r->m_parallel_cf_1.m_delay);
    delay_reset_delay(&r->m_parallel_cf_2.m_delay);
    delay_reset_delay(&r->m_parallel_cf_3.m_delay);
    delay_reset_delay(&r->m_parallel_cf_4.m_delay);
    delay_reset_delay(&r->m_parallel_cf_5.m_delay);
    delay_reset_delay(&r->m_parallel_cf_6.m_delay);
    delay_reset_delay(&r->m_parallel_cf_7.m_delay);
    delay_reset_delay(&r->m_parallel_cf_8.m_delay);

    delay_reset_delay(&r->m_output_apf_3.m_delay);
    delay_reset_delay(&r->m_output_apf_4.m_delay);

    reverb_cook_variables(r);
}

void reverb_cook_variables(reverb *r)
{
    // pre-delay
    delay_set_delay_ms(&r->m_pre_delay, r->m_pre_delay_msec);
    delay_set_output_attenuation_db(&r->m_pre_delay, r->m_pre_delay_atten_db);

    // input diffusion
    delay_set_delay_ms(&r->m_input_apf_1.m_delay, r->m_apf_1_delay_msec);
    delay_apf_set_apf_g(&r->m_input_apf_1, r->m_apf_1_g);

    delay_set_delay_ms(&r->m_input_apf_2.m_delay, r->m_apf_2_delay_msec);
    delay_apf_set_apf_g(&r->m_input_apf_2, r->m_apf_2_g);

    // output diffusion
    delay_set_delay_ms(&r->m_output_apf_3.m_delay, r->m_apf_3_delay_msec);
    delay_apf_set_apf_g(&r->m_output_apf_3, r->m_apf_3_g);

    delay_set_delay_ms(&r->m_output_apf_4.m_delay, r->m_apf_4_delay_msec);
    delay_apf_set_apf_g(&r->m_output_apf_4, r->m_apf_4_g);

    delay_set_delay_ms(&r->m_parallel_cf_1.m_delay, r->m_comb_1_delay_msec);
    delay_set_delay_ms(&r->m_parallel_cf_2.m_delay, r->m_comb_2_delay_msec);
    delay_set_delay_ms(&r->m_parallel_cf_3.m_delay, r->m_comb_3_delay_msec);
    delay_set_delay_ms(&r->m_parallel_cf_4.m_delay, r->m_comb_4_delay_msec);
    delay_set_delay_ms(&r->m_parallel_cf_5.m_delay, r->m_comb_5_delay_msec);
    delay_set_delay_ms(&r->m_parallel_cf_6.m_delay, r->m_comb_6_delay_msec);
    delay_set_delay_ms(&r->m_parallel_cf_7.m_delay, r->m_comb_7_delay_msec);
    delay_set_delay_ms(&r->m_parallel_cf_8.m_delay, r->m_comb_8_delay_msec);

    comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_1, r->m_rt60);
    comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_2, r->m_rt60);
    lpf_comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_3, r->m_rt60);
    lpf_comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_4, r->m_rt60);
    comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_5, r->m_rt60);
    comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_6, r->m_rt60);
    lpf_comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_7, r->m_rt60);
    lpf_comb_filter_set_comb_g_with_rt_sixty(&r->m_parallel_cf_8, r->m_rt60);

    // LPFszz
    one_pole_lpf_set_lpf_g(&r->m_damping_lpf_1, r->m_lpf2_g2);
    one_pole_lpf_set_lpf_g(&r->m_damping_lpf_2, r->m_lpf2_g2);
    one_pole_lpf_set_lpf_g(&r->m_input_lpf, r->m_input_lpf_g);

    // LPF-comb filters
    lpf_comb_filter_set_comb_g(&r->m_parallel_cf_3, r->m_lpf2_g2);
    lpf_comb_filter_set_comb_g(&r->m_parallel_cf_4, r->m_lpf2_g2);
    lpf_comb_filter_set_comb_g(&r->m_parallel_cf_7, r->m_lpf2_g2);
    lpf_comb_filter_set_comb_g(&r->m_parallel_cf_8, r->m_lpf2_g2);
}

stereo_val reverb_process_audio(void *self, stereo_val in)
{
    reverb *r = (reverb *)self;
    double input_sample = in.left + in.right * 0.5;

    double pre_delay_out = 0;
    delay_process_audio(&r->m_pre_delay, &input_sample, &pre_delay_out);

    double apf_1_out = 0;
    delay_apf_process_audio(&r->m_input_apf_1, &pre_delay_out, &apf_1_out);

    double apf_2_out = 0;
    delay_apf_process_audio(&r->m_input_apf_2, &apf_1_out, &apf_2_out);

    double input_lpf = 0;
    one_pole_lpf_process_audio(&r->m_input_lpf, &apf_2_out, &input_lpf);

    double pc_1_out = 0;
    double pc_2_out = 0;
    double pc_3_out = 0;
    double pc_4_out = 0;
    double pc_5_out = 0;
    double pc_6_out = 0;
    double pc_7_out = 0;
    double pc_8_out = 0;
    double c_1_out = 0;
    double c_2_out = 0;

    comb_filter_process_audio(&r->m_parallel_cf_1, &input_lpf, &pc_1_out);
    comb_filter_process_audio(&r->m_parallel_cf_2, &input_lpf, &pc_2_out);
    lpf_comb_filter_process_audio(&r->m_parallel_cf_3, &input_lpf, &pc_3_out);
    lpf_comb_filter_process_audio(&r->m_parallel_cf_4, &input_lpf, &pc_4_out);
    comb_filter_process_audio(&r->m_parallel_cf_5, &input_lpf, &pc_5_out);
    comb_filter_process_audio(&r->m_parallel_cf_6, &input_lpf, &pc_6_out);
    lpf_comb_filter_process_audio(&r->m_parallel_cf_7, &input_lpf, &pc_7_out);
    lpf_comb_filter_process_audio(&r->m_parallel_cf_8, &input_lpf, &pc_8_out);

    c_1_out =
        0.25 * pc_1_out - 0.25 * pc_2_out + 0.25 * pc_3_out - 0.25 * pc_4_out;
    c_2_out =
        0.25 * pc_5_out - 0.25 * pc_6_out + 0.25 * pc_7_out - 0.25 * pc_8_out;

    double damping_lpf_1_out = 0;
    one_pole_lpf_process_audio(&r->m_damping_lpf_1, &c_1_out,
                               &damping_lpf_1_out);

    double damping_lpf_2_out = 0;
    one_pole_lpf_process_audio(&r->m_damping_lpf_2, &c_2_out,
                               &damping_lpf_2_out);

    double apf_3_out = 0;
    delay_apf_process_audio(&r->m_output_apf_3, &damping_lpf_1_out, &apf_3_out);

    double apf_4_out = 0;
    delay_apf_process_audio(&r->m_output_apf_4, &damping_lpf_2_out, &apf_4_out);

    stereo_val out = {};
    out.left = ((100.0 - r->m_wet_pct) / 100.0) * input_sample +
             (r->m_wet_pct / 100.0) * (apf_3_out);

    out.right = ((100.0 - r->m_wet_pct) / 100.0) * input_sample +
             (r->m_wet_pct / 100.0) * (apf_4_out);

    return out;
}

void reverb_status(void *self, char *status_string)
{
    reverb *r = (reverb *)self;
    // clang-format off
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "predelayms:%.2f reverbtime:%.0f wetmx:%.1f",
             r->m_pre_delay_msec, r->m_rt60, r->m_wet_pct);
    // clang-format on
}

void reverb_set_pre_delay_msec(reverb *r, double val)
{
    if (val >= 0 && val <= 100)
        r->m_pre_delay_msec = val;
    else
        printf("Val must be between 0 and 100\n");
}

void reverb_set_pre_delay_atten_db(reverb *r, double val)
{
    if (val >= -96 && val <= 0)
        r->m_pre_delay_atten_db = val;
    else
        printf("Val must be between -96 and 0\n");
}

void reverb_set_rt60(reverb *r, double val)
{
    if (val >= 0 && val <= 5000)
        r->m_rt60 = val;
    else
        printf("Val must be between 0 and 5000\n");
}

void reverb_set_wet_pct(reverb *r, double val)
{
    if (val >= 0 && val <= 100)
        r->m_wet_pct = val;
    else
        printf("Val must be between 0 and 100\n");
}

void reverb_set_input_lpf_g(reverb *r, double val)
{
    if (val >= 0 && val <= 1)
        r->m_input_lpf_g = val;
    else
        printf("Val must be between 0 and 1\n");
}

void reverb_set_lpf2_g2(reverb *r, double val)
{
    if (val >= 0 && val <= 1)
        r->m_lpf2_g2 = val;
    else
        printf("Val must be between 0 and 1\n");
}

void reverb_set_apf_delay_msec(reverb *r, int apf_num, double val)
{
    if (val >= 0 && val <= 100 && apf_num > 0 && apf_num < 5)
    {
        switch (apf_num)
        {
        case (1):
            r->m_apf_1_delay_msec = val;
        case (2):
            r->m_apf_2_delay_msec = val;
        case (3):
            r->m_apf_3_delay_msec = val;
        case (4):
            r->m_apf_4_delay_msec = val;
        }
    }
    else
        printf("Val must be between 0 and 100, apf 1-4\n");
}

void reverb_set_apf_g(reverb *r, int apf_num, double val)
{
    if (val >= -1 && val <= 1 && apf_num > 0 && apf_num < 5)
    {
        switch (apf_num)
        {
        case (1):
            r->m_apf_1_g = val;
        case (2):
            r->m_apf_2_g = val;
        case (3):
            r->m_apf_3_g = val;
        case (4):
            r->m_apf_4_g = val;
        }
    }
    else
        printf("Val must be between -1 and 1, apf 1-4\n");
}

void reverb_set_comb_delay_msec(reverb *r, int comb_num, double val)
{
    if (val >= 0 && val <= 100 && comb_num > 0 && comb_num < 9)
    {
        switch (comb_num)
        {
        case (1):
            r->m_comb_1_delay_msec = val;
        case (2):
            r->m_comb_2_delay_msec = val;
        case (3):
            r->m_comb_3_delay_msec = val;
        case (4):
            r->m_comb_4_delay_msec = val;
        case (5):
            r->m_comb_5_delay_msec = val;
        case (6):
            r->m_comb_6_delay_msec = val;
        case (7):
            r->m_comb_7_delay_msec = val;
        case (8):
            r->m_comb_8_delay_msec = val;
        }
    }
    else
        printf("Val must be between 0 and 100, Combs 1-8\n");
}
