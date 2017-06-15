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
    RESONANTLPF,
    REVERB,
    ALLPASS,
    LOWPASS,
    HIGHPASS,
    BANDPASS,
    QUADFLANGER
} fx_type;

typedef struct fx {
    fx_type type;
    void (*status)(void *self, char *string);
    double (*process)(void *self, double input);
} fx;

#endif
