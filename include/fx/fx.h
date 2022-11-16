#ifndef FX_H
#define FX_H

#include <defjams.h>
#include <stdbool.h>

#include <string>

typedef enum {
  BASICFILTER,
  BITCRUSH,
  COMPRESSOR,
  DECIMATE,
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

class Fx {
 public:
  Fx() = default;
  virtual ~Fx() = default;
  virtual std::string Status() = 0;
  virtual stereo_val Process(stereo_val input) = 0;
  virtual void SetParam(std::string name, double val) = 0;

  virtual void EventNotify(broadcast_event event, mixer_timing_info tinfo);

 public:
  fx_type type_;
  bool enabled_;
};

#endif
