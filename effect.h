#ifndef EFFECT_H
#define EFFECT_H

typedef enum {
    DISTORTION,
    DELAY,
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
    effect_type type;
} EFFECT;

EFFECT* new_delay(double duration, effect_type e_type); 
EFFECT* new_distortion(void);
EFFECT* new_freq_pass(double freq, effect_type pass_type); 

#endif
