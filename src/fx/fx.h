#ifndef FX_H
#define FX_H

#include <defjams.h>
#include <stdbool.h>

#include <string>

enum class fx_type {
  NONE = -1,
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
  WAVESHAPER,
  GRANULATE,
};

class Fx {
 public:
  Fx() = default;
  virtual ~Fx() = default;
  virtual std::string Status() = 0;
  virtual StereoVal Process(StereoVal input) = 0;
  virtual void SetParam(std::string name, double val) = 0;

  virtual void EventNotify(broadcast_event event, mixer_timing_info tinfo);

 public:
  fx_type type_{fx_type::NONE};
  bool enabled_{false};
};

#endif
