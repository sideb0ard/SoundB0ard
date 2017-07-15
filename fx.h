#ifndef FX_H
#define FX_H

#include <stdbool.h>

typedef enum {
    BASICFILTER,
    BEATREPEAT,
    COMPRESSOR,
    DECIMATOR,
    DELAY,
    GRANULATOR,
    MODDELAY,
    MODFILTER,
    DISTORTION,
    RESONANTLPF,
    REVERB,
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
