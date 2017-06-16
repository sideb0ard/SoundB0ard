#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "modfilter.h"

modfilter *new_modfilter()
{
    modfilter *mf = calloc(1, sizeof(modfilter));
    modfilter_init(mf);

    mf->m_fx.type = MODFILTER;
    mf->m_fx.enabled = true;
    mf->m_fx.status = &modfilter_status;
    mf->m_fx.process = &modfilter_process_wrapper;

    return mf;
}

void modfilter_init(modfilter *mf)
{
    biquad_flush_delays(&mf->m_left_lpf);
    biquad_flush_delays(&mf->m_right_lpf);

    wt_initialize(&mf->m_fc_lfo);
    wt_initialize(&mf->m_q_lfo);

    mf->m_min_cutoff_freq = 100.;
    mf->m_max_cutoff_freq = 5000.;
    mf->m_min_q = 0.577;
    mf->m_max_q = 10;

    mf->m_lfo_waveform = sine;
    mf->m_mod_depth_fc = 50.;
    mf->m_mod_rate_fc = 2.5;
    mf->m_mod_depth_q = 50.;
    mf->m_mod_rate_q = 2.5;
    mf->m_lfo_phase = 0;

    mf->m_fc_lfo.polarity = 1; // unipolar
    mf->m_fc_lfo.mode = 0; // normal, not band limited

    mf->m_q_lfo.polarity = 1; // unipolar
    mf->m_q_lfo.mode = 0; // normal, not band limited

    modfilter_update(mf);

    wt_start(&mf->m_fc_lfo);
    wt_start(&mf->m_q_lfo);
}

void modfilter_update(modfilter *mf)
{
    mf->m_fc_lfo.freq = mf->m_mod_rate_fc;
    mf->m_q_lfo.freq = mf->m_mod_rate_q;
    mf->m_fc_lfo.waveform = mf->m_lfo_waveform;
    mf->m_q_lfo.waveform = mf->m_lfo_waveform;
    wt_update(&mf->m_fc_lfo);
    wt_update(&mf->m_q_lfo);
}

double modfilter_calculate_cutoff_freq(modfilter *mf, double lfo_sample)
{
    return (mf->m_mod_depth_fc / 100.0) *
               (lfo_sample * (mf->m_max_cutoff_freq - mf->m_min_cutoff_freq)) +
           mf->m_min_cutoff_freq;
}

double modfilter_calculate_q(modfilter *mf, double lfo_sample)
{
    return (mf->m_mod_depth_q / 100.0) *
               (lfo_sample * (mf->m_max_q - mf->m_min_q)) +
           mf->m_min_q;
}

void modfilter_calculate_left_lpf_coeffs(modfilter *mf, double cutoff_freq,
                                         double q)
{
    double theta_c = 2.0 * M_PI * cutoff_freq / (double)SAMPLE_RATE;
    double d = 1.0 / q;

    double beta_numerator = 1.0 - ((d / 2.0) * (sin(theta_c)));
    double beta_denominator = 1.0 + ((d / 2.0) * (sin(theta_c)));

    double beta = 0.5 * (beta_numerator / beta_denominator);

    double gamma = (0.5 + beta) * (cos(theta_c));

    double alpha = (0.5 + beta - gamma) / 2.0;

    // left channel
    mf->m_left_lpf.m_a0 = alpha;
    mf->m_left_lpf.m_a1 = 2.0 * alpha;
    mf->m_left_lpf.m_a2 = alpha;
    mf->m_left_lpf.m_b1 =
        -2.0 * gamma; // if b's are negative in the difference equation
    mf->m_left_lpf.m_b2 = 2.0 * beta;
}

void modfilter_calculate_right_lpf_coeffs(modfilter *mf, double cutoff_freq,
                                          double q)
{
    double theta_c = 2.0 * M_PI * cutoff_freq / (double)SAMPLE_RATE;
    double d = 1.0 / q;

    double beta_numerator = 1.0 - ((d / 2.0) * (sin(theta_c)));
    double beta_denominator = 1.0 + ((d / 2.0) * (sin(theta_c)));

    double beta = 0.5 * (beta_numerator / beta_denominator);

    double gamma = (0.5 + beta) * (cos(theta_c));

    double alpha = (0.5 + beta - gamma) / 2.0;

    // left channel
    mf->m_right_lpf.m_a0 = alpha;
    mf->m_right_lpf.m_a1 = 2.0 * alpha;
    mf->m_right_lpf.m_a2 = alpha;
    mf->m_right_lpf.m_b1 =
        -2.0 * gamma; // if b's are negative in the difference equation
    mf->m_right_lpf.m_b2 = 2.0 * beta;
}

bool modfilter_process_audio(modfilter *mf, double *in, double *out)
{
    double yn = 0.0;
    double yqn = 0; // quad phase

    yn = wt_do_oscillate(&mf->m_fc_lfo, &yqn);
    double fc = modfilter_calculate_cutoff_freq(mf, yn);
    double fcq = modfilter_calculate_cutoff_freq(mf, yqn);
    // printf("LFO FC: %f FC: %f\n", yn, fc);

    yn = wt_do_oscillate(&mf->m_q_lfo, &yqn);
    double q = modfilter_calculate_q(mf, yn);
    double qq = modfilter_calculate_q(mf, yqn);
    // printf("LFO Q: %f Q: %f\n", yn, q);

    modfilter_calculate_left_lpf_coeffs(mf, fc, q);

    if (mf->m_lfo_phase == 0)
        modfilter_calculate_right_lpf_coeffs(mf, fc, q);
    else
        modfilter_calculate_right_lpf_coeffs(mf, fcq, qq);

    *out = biquad_process(&mf->m_left_lpf, *in);

    // TODO if stereo, use m_right_lp too

    return true;
}

double modfilter_process_wrapper(void *self, double input)
{
    modfilter *mf = (modfilter *)self;
    double output = 0;
    modfilter_process_audio(mf, &input, &output);
    return output;
}
void modfilter_status(void *self, char *status_string)
{
    modfilter *mf = (modfilter *)self;
    snprintf(
        status_string, MAX_PS_STRING_SZ,
        "depthfc:%.2f ratefc:%.2f depthq:%.2f rateq:%.2f lfo:%d phase:%d",
        mf->m_mod_depth_fc, mf->m_mod_rate_fc, mf->m_mod_depth_q,
        mf->m_mod_rate_q, mf->m_lfo_waveform, mf->m_lfo_phase);
}

void modfilter_set_mod_depth_fc(modfilter *mf, double val)
{
    if (val >= 0 && val <= 100)
        mf->m_mod_depth_fc = val;
    else
        printf("Val has to be between 0 and 100\n");

    modfilter_update(mf);
}

void modfilter_set_mod_rate_fc(modfilter *mf, double val)
{
    if (val >= 0.2 && val <= 10)
        mf->m_mod_rate_fc = val;
    else
        printf("Val has to be between 0.2 and 10\n");

    modfilter_update(mf);
}

void modfilter_set_mod_depth_q(modfilter *mf, double val)
{
    if (val >= 0 && val <= 100)
        mf->m_mod_depth_q = val;
    else
        printf("Val has to be between 0 and 100\n");

    modfilter_update(mf);
}

void modfilter_set_mod_rate_q(modfilter *mf, double val)
{
    if (val >= 0.2 && val <= 10)
        mf->m_mod_rate_q = val;
    else
        printf("Val has to be between 0.2 and 10\n");

    modfilter_update(mf);
}

void modfilter_set_lfo_waveform(modfilter *mf, unsigned int val)
{
    if (val < 4)
        mf->m_lfo_waveform = val;
    else
        printf("Val has to be between 0 and 3\n");

    modfilter_update(mf);
}

void modfilter_set_lfo_phase(modfilter *mf, unsigned int val)
{
    if (val < 2)
        mf->m_lfo_phase = val;
    else
        printf("Val has to be between 0 or 1\n");

    modfilter_update(mf);
}
