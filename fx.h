#ifndef FX_H
#define FX_H

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

typedef struct fx {
    fx_type type;
    void (*status)(void *self, char *string);
    double (*process)(void *self, double input);
} fx;

#endif
