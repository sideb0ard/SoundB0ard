#ifndef FX_H
#define FX_H

#include <defjams.h>
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
    stereo_val (*process)(void *self, stereo_val input);
    void (*event_notify)(void *self, broadcast_event event);
} fx;

void fx_noop_event_notify(void *self, broadcast_event event);

#endif
