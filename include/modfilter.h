#pragma once

#include "afx/biquad.h"
#include "fx.h"
#include "wt_oscillator.h"

typedef struct modfilter
{
    fx m_fx; // API
    biquad m_left_lpf;
    biquad m_right_lpf;

    wt_oscillator m_fc_lfo;
    wt_oscillator m_q_lfo;

    double m_min_cutoff_freq;
    double m_max_cutoff_freq;
    double m_min_q;
    double m_max_q;

    double m_mod_depth_fc;
    double m_mod_rate_fc;
    double m_mod_depth_q;
    double m_mod_rate_q;
    unsigned int m_lfo_waveform;
    unsigned int m_lfo_phase;

} modfilter;

modfilter *new_modfilter(void);
void modfilter_init(modfilter *mf);
void modfilter_update(modfilter *mf);
double modfilter_calculate_cutoff_freq(modfilter *mf, double lfo_sample);
double modfilter_calculate_q(modfilter *mf, double lfo_sample);
void modfilter_calculate_left_lpf_coeffs(modfilter *mf, double cutoff_freq,
                                         double q);
void modfilter_calculate_right_lpf_coeffs(modfilter *mf, double cutoff_freq,
                                          double q);
bool modfilter_process_audio(modfilter *mf, double *in, double *out);

double modfilter_process_wrapper(void *self, double input);
void modfilter_status(void *self, char *status_string);

void modfilter_set_mod_depth_fc(modfilter *mf, double val);
void modfilter_set_mod_rate_fc(modfilter *mf, double val);
void modfilter_set_mod_depth_q(modfilter *mf, double val);
void modfilter_set_mod_rate_q(modfilter *mf, double val);
void modfilter_set_lfo_waveform(modfilter *mf, unsigned int val);
void modfilter_set_lfo_phase(modfilter *mf, unsigned int val);
