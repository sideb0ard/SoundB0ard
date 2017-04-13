#pragma once
// cribbed from http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/

enum { bq_type_lowpass,
	   bq_type_highpass,
       bq_type_bandpass,
       bq_type_notch,
       bq_type_peak,
       bq_type_lowshelf,
       bq_type_highshelf
};

typedef struct biquad
{
	unsigned int type;
	double a0, a1, a2, b1, b2;
	double z1, z2;
	//double x1, x2;
	double fc, q, peakgain;
} biquad;

void biquad_init(biquad *b, double fc, double q, double peakgain);
double biquad_process(biquad *b, double in);
void  biquad_update(biquad *b);
