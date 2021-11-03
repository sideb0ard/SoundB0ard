#pragma once

#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <fx/stereodelay.h>
#include <sndfile.h>
#include <soundgenerator.h>
#include <stdbool.h>
#include <wchar.h>

#define DEFAULT_AMP 0.7
#define MAX_CONCURRENT_SAMPLES 10  // arbitrary

typedef struct sample_pos {
  int position{0};
  int playing{0};
  int played{0};
  double audiobuffer_cur_pos{0};
  double audiobuffer_inc{0};
  double audiobuffer_pitch{0};
  double amp{0};
  double speed{0};
  double start_pos_pct{0};
  double end_pos_pct{0};
} sample_pos;

class DrumSampler : public SoundGenerator {
 public:
  DrumSampler(std::string filename);
  ~DrumSampler();
  std::string Info() override;
  std::string Status() override;
  stereo_val genNext() override;
  void start() override;
  void noteOn(midi_event ev) override;
  void SetParam(std::string name, double val) override;
  void pitchBend(midi_event ev) override;
  double GetParam(std::string name) override;

  void ImportFile(std::string filename);

 public:
  bool glitch_mode;
  int glitch_rand_factor;

  sample_pos sample_positions[PPBAR];
  int samples_now_playing[MAX_CONCURRENT_SAMPLES];  // contains midi tick of
                                                    // current samples
  int velocity_now_playing[MAX_CONCURRENT_SAMPLES];

  std::string filename;
  int samplerate;
  int channels;

  EnvelopeGenerator eg;
  bool envelope_enabled;

  bool one_shot{true};

  double *buffer{nullptr};
  int bufsize;
  int buf_end_pos;  // this will always be shorter than bufsize for cutting off
                    // sample earlier
  double buffer_pitch;
  // int buf_num_channels;

  int swing;
  bool started;  // to sync at top of loop
};

int get_a_drumsampler_position(DrumSampler *ss);
void drumsampler_reset_samples(DrumSampler *seq);
void drumsampler_set_pitch(DrumSampler *seq, double v);
void drumsampler_set_cutoff_percent(DrumSampler *seq, unsigned int percent);
void drumsampler_enable_envelope_generator(DrumSampler *ds, bool b);
void drumsampler_set_attack_time(DrumSampler *ds, double val);
void drumsampler_set_decay_time(DrumSampler *ds, double val);
void drumsampler_set_sustain_lvl(DrumSampler *ds, double val);
void drumsampler_set_release_time(DrumSampler *ds, double val);
void drumsampler_set_glitch_mode(DrumSampler *ds, bool b);
void drumsampler_set_glitch_rand_factor(DrumSampler *ds, int pct);
