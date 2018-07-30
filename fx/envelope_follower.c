#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "envelope_follower.h"

envelope_follower *new_envelope_follower()
{
    envelope_follower *ef = calloc(1, sizeof(envelope_follower));
    envelope_follower_init(ef);

    ef->m_fx.type = ENVELOPEFOLLOWER;
    ef->m_fx.enabled = true;
    ef->m_fx.status = &envelope_follower_status;
    ef->m_fx.process = &envelope_follower_process_wrapper;
    ef->m_fx.event_notify = &fx_noop_event_notify;

    envelope_follower_init(ef);

    return ef;
}

void envelope_follower_init(envelope_follower *ef)
{
    ef->m_min_cutoff_freq = 100.;
    ef->m_max_cutoff_freq = 5000.;

    ef->m_pregain_db = 12;   // range 0-20
    ef->m_threshold = 0.2;   // range 0-1
    ef->m_attack_ms = 25;    // 10-100
    ef->m_release_ms = 50;   // 20 - 250
    ef->m_q = 5;             // 0.5 - 20
    ef->m_time_constant = 1; // digital
    ef->m_direction = 0;     // up

    biquad_flush_delays(&ef->m_left_lpf);
    biquad_flush_delays(&ef->m_right_lpf);

    if (ef->m_time_constant == 1) // digital
    {
        envelope_detector_init(&ef->m_left_detector, ef->m_attack_ms,
                               ef->m_release_ms, false, DETECT_MODE_RMS, false);
        envelope_detector_init(&ef->m_right_detector, ef->m_attack_ms,
                               ef->m_release_ms, false, DETECT_MODE_RMS, false);
    }
    else
    {
        envelope_detector_init(&ef->m_left_detector, ef->m_attack_ms,
                               ef->m_release_ms, true, DETECT_MODE_RMS, true);
        envelope_detector_init(&ef->m_right_detector, ef->m_attack_ms,
                               ef->m_release_ms, true, DETECT_MODE_RMS, true);
    }
}

void envelope_follower_update(envelope_follower *ef)
{
    envelope_detector_setattacktime(&ef->m_left_detector, ef->m_attack_ms);
    envelope_detector_setattacktime(&ef->m_right_detector, ef->m_attack_ms);
    envelope_detector_setreleasetime(&ef->m_left_detector, ef->m_release_ms);
    envelope_detector_setreleasetime(&ef->m_right_detector, ef->m_release_ms);
}

double envelope_follower_calculate_cutoff_freq(envelope_follower *ef,
                                               double env_sample)
{
    if (ef->m_direction == 0) // up
        return env_sample * (ef->m_max_cutoff_freq - ef->m_min_cutoff_freq) +
               ef->m_min_cutoff_freq;
    else
        return ef->m_max_cutoff_freq -
               env_sample * (ef->m_max_cutoff_freq - ef->m_min_cutoff_freq);

    return ef->m_min_cutoff_freq; // should never get here
}

void envelope_follower_calculate_left_lpf_coeffs(envelope_follower *ef,
                                                 double cutoff_freq, double q)
{
    double theta_c = 2.0 * M_PI * cutoff_freq / (double)SAMPLE_RATE;
    double d = 1.0 / q;

    double beta_numerator = 1.0 - ((d / 2.0) * (sin(theta_c)));
    double beta_denominator = 1.0 + ((d / 2.0) * (sin(theta_c)));

    double beta = 0.5 * (beta_numerator / beta_denominator);

    double gamma = (0.5 + beta) * (cos(theta_c));

    double alpha = (0.5 + beta - gamma) / 2.0;

    // left channel
    ef->m_left_lpf.m_a0 = alpha;
    ef->m_left_lpf.m_a1 = 2.0 * alpha;
    ef->m_left_lpf.m_a2 = alpha;
    ef->m_left_lpf.m_b1 =
        -2.0 * gamma; // if b's are negative in the difference equation
    ef->m_left_lpf.m_b2 = 2.0 * beta;
}

void envelope_follower_calculate_right_lpf_coeffs(envelope_follower *ef,
                                                  double cutoff_freq, double q)
{
    double theta_c = 2.0 * M_PI * cutoff_freq / (double)SAMPLE_RATE;
    double d = 1.0 / q;

    double beta_numerator = 1.0 - ((d / 2.0) * (sin(theta_c)));
    double beta_denominator = 1.0 + ((d / 2.0) * (sin(theta_c)));

    double beta = 0.5 * (beta_numerator / beta_denominator);

    double gamma = (0.5 + beta) * (cos(theta_c));

    double alpha = (0.5 + beta - gamma) / 2.0;

    // left channel
    ef->m_right_lpf.m_a0 = alpha;
    ef->m_right_lpf.m_a1 = 2.0 * alpha;
    ef->m_right_lpf.m_a2 = alpha;
    ef->m_right_lpf.m_b1 =
        -2.0 * gamma; // if b's are negative in the difference equation
    ef->m_right_lpf.m_b2 = 2.0 * beta;
}

bool envelope_follower_process_audio(envelope_follower *ef, double *in,
                                     double *out)
{
    double gain = pow(10, ef->m_pregain_db / 20.);
    double detect_left =
        envelope_detector_detect(&ef->m_left_detector, gain * (*in));
    double mod_freq_left = ef->m_min_cutoff_freq;

    if (detect_left >= ef->m_threshold)
        mod_freq_left =
            envelope_follower_calculate_cutoff_freq(ef, detect_left);

    envelope_follower_calculate_left_lpf_coeffs(ef, mod_freq_left, ef->m_q);
    *out = biquad_process(&ef->m_left_lpf, *in);

    // TODO right channel stuff

    return true;
}

double envelope_follower_process_wrapper(void *self, double input)
{
    envelope_follower *ef = (envelope_follower *)self;
    double output = 0;
    envelope_follower_process_audio(ef, &input, &output);
    return output;
}
void envelope_follower_status(void *self, char *status_string)
{
    envelope_follower *ef = (envelope_follower *)self;
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "pregain:%.2f threshold:%.2f "
             "attackms:%.2f releasems:%.2f "
             "q:%.2f mode:%s(%d) dir:%s(%d)",
             ef->m_pregain_db, ef->m_threshold, ef->m_attack_ms,
             ef->m_release_ms, ef->m_q,
             ef->m_time_constant ? "DIGITAL" : "ANALOG", ef->m_time_constant,
             ef->m_direction ? "DOWN" : "UP", ef->m_direction);
}

void envelope_follower_set_pregain_db(envelope_follower *ef, double val)
{
    if (val >= 0 && val <= 20)
        ef->m_pregain_db = val;
    else
        printf("Val has to be between 0 and 20\n");
}

void envelope_follower_set_threshold(envelope_follower *ef, double val)
{
    if (val >= 0 && val <= 1)
        ef->m_threshold = val;
    else
        printf("Val has to be between 0 and 1\n");
}

void envelope_follower_set_attack_ms(envelope_follower *ef, double val)
{
    if (val >= 10 && val <= 100)
    {
        ef->m_attack_ms = val;
        envelope_detector_setattacktime(&ef->m_left_detector, val);
        envelope_detector_setattacktime(&ef->m_right_detector, val);
    }
    else
        printf("Val has to be between 10 and 100\n");
}

void envelope_follower_set_release_ms(envelope_follower *ef, double val)
{
    if (val >= 20 && val <= 250)
    {
        ef->m_release_ms = val;
        envelope_detector_setreleasetime(&ef->m_left_detector, val);
        envelope_detector_setreleasetime(&ef->m_right_detector, val);
    }
    else
        printf("Val has to be between 20 and 250\n");
}

void envelope_follower_set_q(envelope_follower *ef, double val)
{
    if (val >= 0.5 && val <= 20)
        ef->m_q = val;
    else
        printf("Val has to be between 0.5 and 20\n");
}

void envelope_follower_set_time_constant(envelope_follower *ef,
                                         unsigned int val)
{
    if (val < 2)
    {
        ef->m_time_constant = val;
        if (val == 0)
        { // analog
            envelope_detector_settcmodeanalog(&ef->m_left_detector, true);
            envelope_detector_settcmodeanalog(&ef->m_right_detector, true);
        }
        else
        {
            envelope_detector_settcmodeanalog(&ef->m_left_detector, false);
            envelope_detector_settcmodeanalog(&ef->m_right_detector, false);
        }
    }
    else
        printf("Val has to be 0 or 1\n");
}

void envelope_follower_set_direction(envelope_follower *ef, unsigned int val)
{
    if (val < 2)
        ef->m_direction = val;
    else
        printf("Val has to be 0 or 1\n");
}
