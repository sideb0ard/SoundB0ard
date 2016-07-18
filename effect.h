#ifndef EFFECT_H
#define EFFECT_H

typedef enum {
    DECIMATOR,
    DELAY1,
    DELAY2,
    DISTORTION,
    RES,
    REVERB,
    ALLPASS,
    LOWPASS,
    HIGHPASS,
    BANDPASS
} effect_type;

typedef struct {
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
} EFFECT;

EFFECT *new_delay(double duration, effect_type e_type);
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
