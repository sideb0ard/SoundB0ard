#include "drumsampler.h"

#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>

#include "defjams.h"
#include "mixer.h"
#include "utils.h"

extern Mixer *mixr;

namespace SBAudio {

DrumSampler::DrumSampler(std::string filename) {
  if (!ImportFile(filename)) {
    // ugh!
    active = false;
    return;
  }

  glitch_mode = false;
  glitch_rand_factor = 20;

  type = DRUMSAMPLER_TYPE;

  eg.SetEgMode(ANALOG);
  eg.SetAttackTimeMsec(0);
  eg.SetDecayTimeMsec(3000);
  eg.SetSustainLevel(0);
  eg.SetReleaseTimeMsec(3000);
  eg.m_output_eg = true;
  eg.ramp_mode = true;
  eg.m_reset_to_zero = true;

  active = true;
  is_playing = false;
}

stereo_val DrumSampler::GenNext(mixer_timing_info tinfo) {
  (void)tinfo;

  double left_val = 0;
  double right_val = 0;
  if (!is_playing) return {left_val, right_val};

  double amp = scaleybum(0, 127, 0, 1, velocity);
  left_val += buffer[play_idx] * amp;

  if (channels == 2)
    right_val += buffer[play_idx + 1] * amp;
  else
    right_val = left_val;

  if (!glitch_mode || glitch_rand_factor > (rand() % 100)) {
    if (reverse_mode)
      play_idx -= channels * buffer_pitch;
    else
      play_idx += channels * buffer_pitch;
    // this ensures play_idx is even
    if (channels == 2) play_idx -= ((int)play_idx & 1);

    if (play_idx >= buf_end_pos) {
      is_playing = false;
    }
    if (play_idx < 0) {
      is_playing = false;
      reverse_mode = false;
    }
  }

  double amp_env = eg.DoEnvelope(NULL);

  float out_vol = volume * amp_env;

  pan = fmin(pan, 1.0);
  pan = fmax(pan, -1.0);
  double pan_left = 0.707;
  double pan_right = 0.707;
  calculate_pan_values(pan, &pan_left, &pan_right);

  stereo_val out = {.left = left_val * out_vol * pan_left,
                    .right = right_val * out_vol * pan_right};
  out = Effector(out);
  return out;
}

std::string DrumSampler::Status() {
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;
  if (!active || volume == 0)
    ss << ANSI_COLOR_RESET;
  else
    ss << COOL_COLOR_PINK;

  ss << "Sampler(" << filename << ")"
     << " vol:" << volume << " pan:" << pan << " pitch:" << buffer_pitch
     << ANSI_COLOR_RESET;

  return ss.str();
}

std::string DrumSampler::Info() {
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;
  ss << ANSI_COLOR_BLUE << "\nSample ";
  ss << ANSI_COLOR_WHITE << filename << ANSI_COLOR_BLUE;
  ss << "\nvol:" << volume << " pan:" << pan << " pitch:" << buffer_pitch;
  ss << " attack_ms:" << eg.m_attack_time_msec;
  ss << " decay_ms:" << eg.m_decay_time_msec
     << " release_ms:" << eg.m_release_time_msec;
  ss << "\nreverse:" << reverse_mode << " glitch:" << glitch_mode
     << " gpct:" << glitch_rand_factor;
  return ss.str();
}

void DrumSampler::start() {
  if (active) return;  // no-op
  active = true;
}

void DrumSampler::noteOn(midi_event ev) {
  is_playing = true;
  play_idx = 0;
  if (reverse_mode) play_idx = bufsize - 2;
  velocity = ev.data2;
  eg.StartEg();
}

void DrumSampler::noteOff(midi_event ev) { is_playing = false; }

void DrumSampler::pitchBend(midi_event ev) {
  float pitch_val = ev.data1 / 10.;
  SetPitch(pitch_val);
}

bool DrumSampler::ImportFile(std::string fname) {
  AudioBufferDetails deetz = ImportFileContents(buffer, fname);
  if (deetz.buffer_length == 0) return false;

  filename = deetz.filename;
  bufsize = deetz.buffer_length;
  buf_end_pos = bufsize;
  buffer_pitch = 1.0;
  samplerate = deetz.sample_rate;
  channels = deetz.num_channels;
  return true;
}

void DrumSampler::SetPitch(double v) {
  if (v >= 0. && v <= 2.0)
    buffer_pitch = v;
  else
    printf("Must be in the range of 0.0 .. 2.0\n");
}

void DrumSampler::SetAttackTime(double val) { eg.SetAttackTimeMsec(val); }
void DrumSampler::SetDecayTime(double val) { eg.SetDecayTimeMsec(val); }
void DrumSampler::SetSustainLvl(double val) { eg.SetSustainLevel(val); }
void DrumSampler::SetReleaseTime(double val) { eg.SetReleaseTimeMsec(val); }

void DrumSampler::SetGlitchMode(bool b) { glitch_mode = b; }

void DrumSampler::SetGlitchRandFactor(int pct) {
  if (pct >= 0 && pct <= 100) glitch_rand_factor = pct;
}
void DrumSampler::SetParam(std::string name, double val) {
  if (name == "pitch")
    SetPitch(val);
  else if (name == "attack_ms")
    SetAttackTime(val);
  else if (name == "decay_ms")
    SetDecayTime(val);
  else if (name == "release_ms")
    SetReleaseTime(val);
  else if (name == "reverse")
    reverse_mode = val;
  else if (name == "glitch")
    glitch_mode = val;
  else if (name == "gpct") {
    if (val >= 0 && val <= 100) glitch_rand_factor = val;
  }
}

}  // namespace SBAudio
