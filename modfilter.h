#pragma once

#include "lfo.h"
#include "afx/biquad_lpf.h"

typedef struct modfilter
{
    biquad m_left_lpf;
    biquad m_right_lpf;

    lfo m_fc_lfo;
    lfo m_q_lfo;

    double m_min_cutoff_freq;
    double m_max_cutoff_freq;
    double m_min_q;
    double m_max_q;

    double m_mod_depth_fc;
    double m_mod_rate_fc;
    unsigned int m_lfo_waveform;
    double m_mod_depth_q;
    double m_mod_rate_q;
    unsigned int m_lfo_phase;

} modfilter;

modfilter *new_modfilter(void);
void modfilter_init(modfilter *mf);
double modfilter_calculate_cutoff_freq(modfilter *mf, double lfo_sample);
double modfilter_calculate_q(modfilter *mf, double lfo_sample);
void modfilter_calculate_left_lpf_coeffs(modfilter *mf, double cutoff_freq, double q);
void modfilter_calculate_right_lpf_coeffs(modfilter *mf, double cutoff_freq, double q);
bool modfilter_process_audio(modfilter *mf, double *in, double *out);
