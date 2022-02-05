#pragma once

#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <fx/stereodelay.h>
#include <sndfile.h>
#include <soundgenerator.h>
#include <stdbool.h>

constexpr double kDefaultAmp = 0.7;
constexpr int kMaxConcurrentSamples = 10;  // arbitrary

class DrumSampler : public SoundGenerator {
 public:
  DrumSampler(std::string filename);
  ~DrumSampler() = default;
  std::string Info() override;
  std::string Status() override;
  stereo_val genNext(mixer_timing_info tinfo) override;
  void start() override;
  void noteOn(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  void pitchBend(midi_event ev) override;
  double GetParam(std::string name) override;

  void ImportFile(std::string filename);

 public:
  bool glitch_mode;
  int glitch_rand_factor;

  bool is_playing{false};
  float play_idx{0};
  int velocity{127};
  int reverse_mode{0};
  float playback_speed{1};

  std::string filename;
  int samplerate;
  int channels;

  EnvelopeGenerator eg;

  std::vector<double> buffer{};
  int bufsize;
  int buf_end_pos;  // this will always be shorter than bufsize for cutting off
                    // sample earlier
  double buffer_pitch;
  // int buf_num_channels;

 private:
  void SetPitch(double v);
  void SetAttackTime(double val);
  void SetDecayTime(double val);
  void SetSustainLvl(double val);
  void SetReleaseTime(double val);
  void SetGlitchMode(bool b);
  void SetGlitchRandFactor(int pct);
};
