#pragma once

#include <sndfile.h>
#include <stdbool.h>

#include <memory>

#include "envelope_generator.h"
#include "filebuffer.h"
#include "filter_moogladder.h"
#include "fx/stereodelay.h"
#include "soundgenerator.h"

namespace SBAudio {

constexpr double kDefaultAmp = 0.7;
constexpr int kMaxConcurrentSamples = 10;  // arbitrary

class DrumSampler : public SoundGenerator {
 public:
  DrumSampler(std::string filename);
  ~DrumSampler() = default;
  std::string Info() override;
  std::string Status() override;
  StereoVal GenNext(mixer_timing_info tinfo) override;
  void Start() override;
  void NoteOn(midi_event ev) override;
  void NoteOff(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  void PitchBend(midi_event ev) override;

  bool ImportFile(std::string filename);

 public:
  bool reverse_mode{false};

  bool stop_pending_{false};
  bool is_playing{false};
  int velocity{127};

  EnvelopeGenerator eg;

  std::unique_ptr<FileBuffer> file_buffer_;
  double incr_{1};
  double read_position_{0};  // Fractional read position for interpolation

 private:
  std::string cached_filename_;  // Cache to avoid reading
                                 // file_buffer_->filename_ from audio thread

  void SetPitch(double v);
  void SetAttackTime(double val);
  void SetDecayTime(double val);
  void SetSustainLvl(double val);
  void SetReleaseTime(double val);
};

}  // namespace SBAudio
