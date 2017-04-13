#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "defjams.h"
#include "modfilter.h"

modfilter *new_modfilter()
{
    modfilter *mf = calloc(1, sizeof(modfilter));
    modfilter_init(mf);
    return mf;
}

void modfilter_init(modfilter *mf)
{
    biquad_init(&mf->m_left_lpf, 1200. / SAMPLE_RATE, 8.0, 0.0);
    //biquad_init(&mf->m_right_lpf);

    osc_new_settings((oscillator *)&mf->m_fc_lfo);
    lfo_set_soundgenerator_interface(&mf->m_fc_lfo);
    lfo_start_oscillator((oscillator *)&mf->m_fc_lfo);

    osc_new_settings((oscillator *)&mf->m_q_lfo);
    lfo_set_soundgenerator_interface(&mf->m_q_lfo);
    lfo_start_oscillator((oscillator *)&mf->m_q_lfo);

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
}

double modfilter_calculate_cutoff_freq(modfilter *mf, double lfo_sample)
{
    return (mf->m_mod_depth_q/100.0)*(lfo_sample*(mf->m_max_cutoff_freq)) + mf->m_min_cutoff_freq;
}

double modfilter_calculate_q(modfilter *mf, double lfo_sample)
{
    return (mf->m_mod_depth_q/100.0)*(lfo_sample*(mf->m_max_q - mf->m_min_q)) + mf->m_min_q;
}

void modfilter_calculate_left_lpf_coeffs(modfilter *mf, double cutoff_freq, double q)
{
    double theta_c = 2.0*M_PI*cutoff_freq/(double)SAMPLE_RATE;
    double d = 1.0/q;

    double beta_numerator =   1.0 - ((d/2.0)*(sin(theta_c)));
    double beta_denominator = 1.0 + ((d/2.0)*(sin(theta_c)));

    double beta = 0.5*(beta_numerator/beta_denominator);

    double gamma = (0.5 + beta)*(cos(theta_c));

    double alpha = (0.5 + beta - gamma)/2.0;

    // left channel
    mf->m_left_lpf.a0 = alpha;
    mf->m_left_lpf.a1 = 2.0*alpha;
    mf->m_left_lpf.a2 = alpha;
    mf->m_left_lpf.b1 = -2.0*gamma; // if b's are negative in the difference equation
    mf->m_left_lpf.b2 = 2.0*beta;
}

void modfilter_calculate_right_lpf_coeffs(modfilter *mf, double cutoff_freq, double q)
{
    double theta_c = 2.0*M_PI*cutoff_freq/(double)SAMPLE_RATE;
    double d = 1.0/q;

    double beta_numerator =   1.0 - ((d/2.0)*(sin(theta_c)));
    double beta_denominator = 1.0 + ((d/2.0)*(sin(theta_c)));

    double beta = 0.5*(beta_numerator/beta_denominator);

    double gamma = (0.5 + beta)*(cos(theta_c));

    double alpha = (0.5 + beta - gamma)/2.0;

    // left channel
    mf->m_right_lpf.a0 = alpha;
    mf->m_right_lpf.a1 = 2.0*alpha;
    mf->m_right_lpf.a2 = alpha;
    mf->m_right_lpf.b1 = -2.0*gamma; // if b's are negative in the difference equation
    mf->m_right_lpf.b2 = 2.0*beta;

}

bool modfilter_process_audio(modfilter *mf, double *in, double *out)
{
    double yn = 0.0;
    double yqn = 0; // quad phase

    osc_update((oscillator *)&mf->m_fc_lfo, "LFO1FC");
    yn = lfo_do_oscillate((oscillator *)&mf->m_fc_lfo, &yqn);
    double fc = modfilter_calculate_cutoff_freq(mf, yn);
    //printf("LFO FC: %f FC: %f\n", yn, fc);
    //double fcq = modfilter_calculate_cutoff_freq(mf, yqn);

    osc_update((oscillator *)&mf->m_q_lfo, "LFO1Q");
    yn = lfo_do_oscillate((oscillator *)&mf->m_q_lfo, &yqn);
    double q = modfilter_calculate_q(mf, yn);
    //printf("LFO Q: %f Q: %f\n", yn, q);
    //double qq = modfilter_calculate_q(mf, yqn);

    modfilter_calculate_left_lpf_coeffs(mf, fc, q);

    //if (mf->m_lfo_phase == 0)
    //    modfilter_calculate_right_lpf_coeffs(mf, fc, q);
    //else
    //    modfilter_calculate_right_lpf_coeffs(mf, fcq, qq);

    //biquad_update(&mf->m_left_lpf);
    *out = biquad_process(&mf->m_left_lpf, *in);
    printf("IN: %f OUT: %f\n", *in, *out);

    // TODO if stereo, use m_right_lp too


    return true;

}
