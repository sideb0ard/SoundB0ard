#ifndef EFFECT_H
#define EFFECT_H

#include "modfilter.h"
#include "modular_delay.h"
#include "reverb.h"
#include "stereodelay.h"

typedef enum {
    BEATREPEAT,
    DECIMATOR,
    DELAY,
    MODDELAY,
    MODFILTER,
    DISTORTION,
    RES,
    REVERB,
    ALLPASS,
    LOWPASS,
    HIGHPASS,
    BANDPASS,
} fx_type;

typedef struct {
    fx_type type;
    void (*status)(void *self, char *string);
    double (*process)(void *self, double input);
} fx;

fx *new_beatrepeat(int looplen);
fx *new_delay(double duration);
fx *effect_new_mod_delay(void);
fx *effect_new_mod_filter(void);
fx *new_reverb_effect(void);
fx *new_decimator(void);
fx *new_distortion(void);
fx *new_freq_pass(double freq, fx_type pass_type);

void cook_variables(fx *self);
void set_delay_ms(fx *self, double msec);
double read_delay(fx *self);
double read_delay_at(fx *self, double msec);
void write_delay_and_inc(fx *self, double val);
void delay_audio(double *input, double *output);

#endif
