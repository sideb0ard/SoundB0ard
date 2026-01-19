#ifndef FX_H
#define FX_H

#include <defjams.h>
#include <stdbool.h>

#include <string>

enum class fx_type {
  NONE = -1,

  // === SATURATION/DISTORTION ===
  DISTORTION,  // Enhanced multi-mode distortion (hard, soft, tube, foldback)
  WAVESHAPER,  // Arctan saturation (kept for compatibility)

  // === DEGRADATION/LOFI ===
  LOFI_CRUSHER,  // Unified bitcrush/decimate (bit depth + sample rate
                 // reduction)

  // === TIME-BASED ===
  DELAY,
  MODDELAY,
  REVERB,
  TRANSVERB,

  // === CREATIVE/EXPERIMENTAL ===
  WAVEFORM_SCULPTOR,  // Landmark-based waveform manipulation (was Geometer)
  DIFFUSER,           // Multi-stage diffusion/blur/reverb (was Nnirror)

  // === DYNAMICS ===
  COMPRESSOR,

  // === FILTERS ===
  BASICFILTER,
  MODFILTER,
  RESONANTLPF,

  // === OTHER ===
  ENVELOPE,
  QUADFLANGER,
  ENVELOPEFOLLOWER,
  GRANULATE,

  // === DEPRECATED (for backward compatibility) ===
  BITCRUSH = LOFI_CRUSHER,      // Alias to LOFI_CRUSHER
  DECIMATE = LOFI_CRUSHER,      // Alias to LOFI_CRUSHER
  NNIRROR = DIFFUSER,           // Alias to DIFFUSER
  GEOMETER = WAVEFORM_SCULPTOR  // Alias to WAVEFORM_SCULPTOR
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
