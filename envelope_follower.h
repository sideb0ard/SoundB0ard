#pragma once

#include "afx/biquad.h"
#include "fx.h"
#include "envelope_detector.h"

typedef struct envelope_follower {
    fx m_fx; // API
    biquad m_left_lpf;
    biquad m_right_lpf;

    envelope_detector m_left_detector;
    envelope_detector m_right_detector;

    double m_min_cutoff_freq;
    double m_max_cutoff_freq;

    double m_pregain_db;
    double m_threshold;
    double m_attack_ms;
    double m_release_ms;
    double m_q;
    unsigned int m_time_constant; // analog or digital
    unsigned int m_direction; // up down

} envelope_follower;

envelope_follower *new_envelope_follower(void);
void envelope_follower_init(envelope_follower *ef);
void envelope_follower_update(envelope_follower *ef);

double envelope_follower_calculate_cutoff_freq(envelope_follower *ef,
                                               double env_sample);
void envelope_follower_calculate_left_lpf_coeffs(envelope_follower *ef,
                                                 double cutoff_freq, double q);
void envelope_follower_calculate_right_lpf_coeffs(envelope_follower *ef,
                                                  double cutoff_freq, double q);
bool envelope_follower_process_audio(envelope_follower *ef, double *in,
                                     double *out);

double envelope_follower_process_wrapper(void *self, double input);
void envelope_follower_status(void *self, char *status_string);

void envelope_follower_set_pregain_db(envelope_follower *ef, double val);
void envelope_follower_set_threshold(envelope_follower *ef, double val);
void envelope_follower_set_attack_ms(envelope_follower *ef, double val);
void envelope_follower_set_release_ms(envelope_follower *ef, double val);
void envelope_follower_set_q(envelope_follower *ef, double val);
void envelope_follower_set_time_constant(envelope_follower *ef,
                                         unsigned int val);
void envelope_follower_set_direction(envelope_follower *ef, unsigned int val);
