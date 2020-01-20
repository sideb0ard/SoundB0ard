#ifndef FX_H
#define FX_H

#include <defjams.h>
#include <stdbool.h>

typedef enum
{
    BASICFILTER,
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

class Fx
{
  public:
    Fx();
    virtual ~Fx() = default;
    virtual void Status(char *string) = 0;
    virtual stereo_val Process(stereo_val input) = 0;
    virtual void SetParam(std::string name, double val) = 0;
    virtual double GetParam(std::string name) = 0;

    virtual void EventNotify(broadcast_event event);

  public:
    fx_type type_;
    bool enabled_;
};

#endif
