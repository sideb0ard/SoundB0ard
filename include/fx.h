#ifndef FX_H
#define FX_H

#include <stdbool.h>

typedef enum
{
    BASICFILTER,
    BEATREPEAT,
    BITCRUSH,
    COMPRESSOR,
    DECIMATOR,
    DELAY,
    ENVELOPE,
    MODDELAY,
    MODFILTER,
    DISTORTION,
    RESONANTLPF,
    REVERB,
    QUADFLANGER,
    ENVELOPEFOLLOWER,
    WAVESHAPER
} fx_type;

typedef struct fx
{
    fx_type type;
    bool enabled;
    void (*status)(void *self, char *string);
    double (*process)(void *self, double input);
    void (*event_notify)(void *self, unsigned int event_type);
} fx;

void fx_noop_event_notify(void *self, unsigned int event_type);

#endif
