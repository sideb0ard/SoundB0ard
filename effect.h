#ifndef EFFECT_H
#define EFFECT_H

#include "modular_delay.h"
#include "reverb.h"
#include "stereodelay.h"

typedef enum {
    BEATREPEAT,
    DECIMATOR,
    DELAY,
    MODDELAY,
    DISTORTION,
    RES,
    REVERB,
    ALLPASS,
    LOWPASS,
    HIGHPASS,
    BANDPASS,
} effect_type;

typedef struct {
    stereodelay *delay;
    mod_delay *moddelay;
    reverb *r;
    double *buffer;
    int buf_read_idx;
    int buf_write_idx;
    double m_duration;
    double m_delay_ms;
    int m_delay_in_samples;
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
    void (*status)(void *self, char *string);
} EFFECT;

EFFECT *new_beatrepeat(int looplen);
EFFECT *new_delay(double duration);
EFFECT *effect_new_mod_delay(void);
EFFECT *new_reverb_effect(void);
EFFECT *new_decimator(void);
EFFECT *new_distortion(void);
EFFECT *new_freq_pass(double freq, effect_type pass_type);

void cook_variables(EFFECT *self);
void set_delay_ms(EFFECT *self, double msec);
double read_delay(EFFECT *self);
double read_delay_at(EFFECT *self, double msec);
void write_delay_and_inc(EFFECT *self, double val);
void delay_audio(double *input, double *output);

#endif
