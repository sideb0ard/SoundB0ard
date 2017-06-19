#ifndef FX_H
#define FX_H

#include <stdbool.h>

typedef enum {
    BASICFILTER,
    BEATREPEAT,
    COMPRESSOR,
    DECIMATOR,
    DELAY,
    MODDELAY,
    MODFILTER,
    DISTORTION,
    // RES,
    RESONANTLPF,
    REVERB,
    // ALLPASS,
    // LOWPASS,
    // HIGHPASS,
    // BANDPASS,
    QUADFLANGER,
    ENVELOPEFOLLOWER,
    WAVESHAPER
} fx_type;

typedef struct fx {
    fx_type type;
    bool enabled;
    void (*status)(void *self, char *string);
    double (*process)(void *self, double input);
} fx;

#endif
