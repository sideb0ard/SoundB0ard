#ifndef EFFECT_H
#define EFFECT_H

typedef enum {
    DECIMATOR,
    DELAY,
    DISTORTION,
    RES,
    REVERB,
    ALLPASS,
    LOWPASS,
    HIGHPASS,
    BANDPASS
} effect_type;

typedef struct {
    double* buffer;
    int buf_p;
    int buf_length;
    double costh;
    double coef;
    double rr;
    double rsq;
    double scal;
    // for decimator
    int bits;
    double rate, cnt;
    long m;
    effect_type type;
} EFFECT;

EFFECT* new_delay(double duration, effect_type e_type); 
EFFECT* new_decimator(void);
EFFECT* new_distortion(void);
EFFECT* new_freq_pass(double freq, effect_type pass_type); 

#endif
