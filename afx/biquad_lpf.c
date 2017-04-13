#include <math.h>
#include <stdio.h>
#include "biquad_lpf.h"
// cribbed from http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/

void biquad_init(biquad *b)
{
    b->type = bq_type_lowpass;
    b->a0 = 1.0;
    b->a1 = 0.0;
    b->a2 = 0.0;
    b->b1 = 0.0;
    b->b1 = 0.0;
    b->q  = 0.707;
    b->fc = 0.50;
    b->peakgain = 0.0;
    b->z1 = 0.0;
    b->z2 = 0.0;
}

void  biquad_update(biquad *b)
{
	double norm;
    double V = pow(10, fabs(b->peakgain) / 20.0);
    double K = tan(M_PI * b->fc);

    switch (b->type) {
        case bq_type_lowpass:
            norm = 1 / (1 + K / b->q + K * K);
            b->a0 = K * K * norm;
            b->a1 = 2 * b->a0;
            b->a2 = b->a0;
            b->b1 = 2 * (K * K - 1) * norm;
            b->b2 = (1 - K / b->q + K * K) * norm;
            break;
            
        case bq_type_highpass:
            norm = 1 / (1 + K / b->q + K * K);
            b->a0 = 1 * norm;
            b->a1 = -2 * b->a0;
            b->a2 = b->a0;
            b->b1 = 2 * (K * K - 1) * norm;
            b->b2 = (1 - K / b->q + K * K) * norm;
            break;
            
        case bq_type_bandpass:
            norm = 1 / (1 + K / b->q + K * K);
            b->a0 = K / b->q * norm;
            b->a1 = 0;
            b->a2 = -b->a0;
            b->b1 = 2 * (K * K - 1) * norm;
            b->b2 = (1 - K / b->q + K * K) * norm;
            break;
            
        case bq_type_notch:
            norm = 1 / (1 + K / b->q + K * K);
            b->a0 = (1 + K * K) * norm;
            b->a1 = 2 * (K * K - 1) * norm;
            b->a2 = b->a0;
            b->b1 = b->a1;
            b->b2 = (1 - K / b->q + K * K) * norm;
            break;
            
        case bq_type_peak:
            if (b->peakgain >= 0) {    // boost
                norm = 1 / (1 + 1/b->q * K + K * K);
                b->a0 = (1 + V/b->q * K + K * K) * norm;
                b->a1 = 2 * (K * K - 1) * norm;
                b->a2 = (1 - V/b->q * K + K * K) * norm;
                b->b1 = b->a1;
                b->b2 = (1 - 1/b->q * K + K * K) * norm;
            }
            else {    // cut
                norm = 1 / (1 + V/b->q * K + K * K);
                b->a0 = (1 + 1/b->q * K + K * K) * norm;
                b->a1 = 2 * (K * K - 1) * norm;
                b->a2 = (1 - 1/b->q * K + K * K) * norm;
                b->b1 = b->a1;
                b->b2 = (1 - V/b->q * K + K * K) * norm;
            }
            break;
        case bq_type_lowshelf:
            if (b->peakgain >= 0) {    // boost
                norm = 1 / (1 + sqrt(2) * K + K * K);
                b->a0 = (1 + sqrt(2*V) * K + V * K * K) * norm;
                b->a1 = 2 * (V * K * K - 1) * norm;
                b->a2 = (1 - sqrt(2*V) * K + V * K * K) * norm;
                b->b1 = 2 * (K * K - 1) * norm;
                b->b2 = (1 - sqrt(2) * K + K * K) * norm;
            }
            else {    // cut
                norm = 1 / (1 + sqrt(2*V) * K + V * K * K);
                b->a0 = (1 + sqrt(2) * K + K * K) * norm;
                b->a1 = 2 * (K * K - 1) * norm;
                b->a2 = (1 - sqrt(2) * K + K * K) * norm;
                b->b1 = 2 * (V * K * K - 1) * norm;
                b->b2 = (1 - sqrt(2*V) * K + V * K * K) * norm;
            }
            break;
        case bq_type_highshelf:
            if (b->peakgain >= 0) {    // boost
                norm = 1 / (1 + sqrt(2) * K + K * K);
                b->a0 = (V + sqrt(2*V) * K + K * K) * norm;
                b->a1 = 2 * (K * K - V) * norm;
                b->a2 = (V - sqrt(2*V) * K + K * K) * norm;
                b->b1 = 2 * (K * K - 1) * norm;
                b->b2 = (1 - sqrt(2) * K + K * K) * norm;
            }
            else {    // cut
                norm = 1 / (V + sqrt(2*V) * K + K * K);
                b->a0 = (1 + sqrt(2) * K + K * K) * norm;
                b->a1 = 2 * (K * K - 1) * norm;
                b->a2 = (1 - sqrt(2) * K + K * K) * norm;
                b->b1 = 2 * (K * K - V) * norm;
                b->b2 = (V - sqrt(2*V) * K + K * K) * norm;
            }
            break;
    }
}

double biquad_process(biquad *b, double in)
{
    double out = in * b->a0 + b->z1;
    b->z1 = in * b->a1 + b->z2 - b->b1 * out;
    b->z2 = in * b->a2 - b->b2 * out;
    return out;
}
