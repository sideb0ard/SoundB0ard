#pragma once

#include "biquad.h"
#include "../fx.h"

typedef struct resonant_lpf
{
    fx m_fx; // API

    biquad m_left_lpf;
    biquad m_right_lpf;

    double m_fc_hz;
    double m_q;

} resonant_lpf;

void resonant_lpf_reset(resonant_lpf *lpf);
void resonant_lpf_calculate_lpf_coeffs(resonant_lpf *lpf, double cutoff, double q);

void resonant_lpf_status(void *self, char *status_string);
double resonant_lpf_process(void *self, double input);


