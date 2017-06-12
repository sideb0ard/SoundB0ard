#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "resonant_lpf.h"
#include "../defjams.h"

resonant_lpf *new_resonant_lpf(void)
{
    resonant_lpf *lpf = calloc(1, sizeof(resonant_lpf));
    resonant_lpf_reset(lpf);

    lpf->m_fx.type = RESONANTLPF;
    lpf->m_fx.status = &resonant_lpf_status;
    lpf->m_fx.process = &resonant_lpf_process;

    return lpf;
}

void resonant_lpf_reset(resonant_lpf *lpf)
{
    biquad_flush_delays(&lpf->m_left_lpf);
    biquad_flush_delays(&lpf->m_right_lpf);

    resonant_lpf_calculate_lpf_coeffs(lpf, lpf->m_fc_hz, lpf->m_q);

}

void resonant_lpf_calculate_lpf_coeffs(resonant_lpf *lpf, double cutoff, double q)
{
	double theta_c = 2.0*M_PI*cutoff/SAMPLE_RATE;
	double d = 1.0/q;
	
	// intermediate values
	double beta_numerator =   1.0 - ((d/2.0)*(sin(theta_c)));
	double beta_denominator = 1.0 + ((d/2.0)*(sin(theta_c)));
	
	// beta
	double beta = 0.5*(beta_numerator/beta_denominator);
	
	// gamma
	double gamma = (0.5 + beta)*(cos(theta_c));
	
	// alpha
	double alpha = (0.5 + beta - gamma)/2.0;
	
	// left channel
	lpf->m_left_lpf.m_a0 = alpha;
	lpf->m_left_lpf.m_a1 = 2.0*alpha;
	lpf->m_left_lpf.m_a2 = alpha;
	lpf->m_left_lpf.m_b1 = -2.0*gamma;
	lpf->m_left_lpf.m_b2 = 2.0*beta;
	
	// right channel
	lpf->m_right_lpf.m_a0 = alpha;
	lpf->m_right_lpf.m_a1 = 2.0*alpha;
	lpf->m_right_lpf.m_a2 = alpha;
	lpf->m_right_lpf.m_b1 = -2.0*gamma;
	lpf->m_right_lpf.m_b2 = 2.0*beta;

}


void resonant_lpf_status(void *self, char *status_string)
{
	resonant_lpf *lpf = (resonant_lpf*) self;
	snprintf(status_string, MAX_PS_STRING_SZ, "fc:%.2f q:%.2f", lpf->m_fc_hz, lpf->m_q);
}

double resonant_lpf_process(void *self, double input)
{
	resonant_lpf *lpf = (resonant_lpf*) self;
	return biquad_process(&lpf->m_left_lpf, input);

    // TODO
    // if stereo, do right lpf too
}
