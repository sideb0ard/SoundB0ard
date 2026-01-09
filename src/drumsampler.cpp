#include "drumsampler.h"

#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <memory>

#include "defjams.h"
#include "mixer.h"
#include "utils.h"

extern std::unique_ptr<Mixer> global_mixr;

namespace SBAudio {

DrumSampler::DrumSampler(std::string filename) {
  // Cache filename to avoid reading from file_buffer_ in audio thread
  cached_filename_ = filename;

  if (!ImportFile(filename)) {
    // ugh!
    active = false;
    return;
  }

  type = DRUMSAMPLER_TYPE;

  eg.SetEgMode(ANALOG);
  eg.SetAttackTimeMsec(0);
  eg.SetDecayTimeMsec(30);
  eg.SetSustainLevel(1);
  eg.SetReleaseTimeMsec(20);
  eg.m_output_eg = true;
  // eg.ramp_mode = true;
  eg.m_reset_to_zero = true;

  active = true;
  is_playing = false;
}

StereoVal DrumSampler::GenNext(mixer_timing_info tinfo) {
  (void)tinfo;

  StereoVal out = {0., 0.};
  if (!is_playing) return out;

  if (stop_pending_ && eg.m_state == OFFF) {
    is_playing = false;
    return out;
  }

  double amp_env = eg.DoEnvelope(NULL);
  if (amp_env == 0) {
    is_playing = false;
    return out;
  }

  std::vector<double> *audio_buffer = file_buffer_->GetAudioBuffer();
  double amp = scaleybum(0, 127, 0, 1, velocity);

  // Variable-rate playback with linear interpolation
  double pitch_ratio = file_buffer_->pitch_ratio_.load();
  int num_channels = file_buffer_->num_channels_;

  // Check bounds
  if (read_position_ < 0 ||
      read_position_ >= audio_buffer->size() - num_channels) {
    is_playing = false;
    if (read_position_ < 0) {
      reverse_mode = false;
    }
    return out;
  }

  // Get integer and fractional parts for interpolation
  int idx1 = (int)read_position_;
  double frac = read_position_ - idx1;
  int idx2 = idx1 + num_channels;

  // Bounds check for interpolation
  if (idx2 >= audio_buffer->size()) {
    is_playing = false;
    return out;
  }

  // Linear interpolation
  if (num_channels == 1) {
    out.left =
        (*audio_buffer)[idx1] * (1.0 - frac) + (*audio_buffer)[idx2] * frac;
    out.left *= amp;
    out.right = out.left;
  } else if (num_channels == 2) {
    out.left =
        (*audio_buffer)[idx1] * (1.0 - frac) + (*audio_buffer)[idx2] * frac;
    out.right = (*audio_buffer)[idx1 + 1] * (1.0 - frac) +
                (*audio_buffer)[idx2 + 1] * frac;
    out.left *= amp;
    out.right *= amp;
  }

  // Advance read position by pitch ratio
  read_position_ += (incr_ * pitch_ratio * num_channels);

  float out_vol = volume * amp_env;

  pan = fmin(pan, 1.0);
  pan = fmax(pan, -1.0);
  double pan_left = 0.707;
  double pan_right = 0.707;
  calculate_pan_values(pan, &pan_left, &pan_right);

  out.left *= out_vol * pan_left;
  out.right *= out_vol * pan_right;

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
  // Use cached_filename_ and atomic load to avoid mutex/race
  ss << "Sampler(" << cached_filename_ << ")"
     << " vol:" << volume << " pan:" << pan
     << " pitch:" << file_buffer_->pitch_ratio_.load() << ANSI_COLOR_RESET;

  return ss.str();
}

std::string DrumSampler::Info() {
  std::stringstream ss;
  ss << std::setprecision(2) << std::fixed;
  ss << ANSI_COLOR_BLUE << "\nSample ";
  // Use cached_filename_ and atomic load to avoid mutex/race
  ss << ANSI_COLOR_WHITE << cached_filename_ << ANSI_COLOR_BLUE;
  ss << "\nvol:" << volume << " pan:" << pan
     << " pitch:" << file_buffer_->pitch_ratio_.load();
  ss << " attack_ms:" << eg.m_attack_time_msec;
  ss << " decay_ms:" << eg.m_decay_time_msec
     << " release_ms:" << eg.m_release_time_msec;
  ss << "\nreverse:" << reverse_mode;
  return ss.str();
}

void DrumSampler::Start() {
  if (active) return;  // no-op
  active = true;
}

void DrumSampler::NoteOn(midi_event ev) {
  is_playing = true;
  read_position_ = 0;
  if (reverse_mode) {
    read_position_ = file_buffer_->GetAudioBuffer()->size() - 2;
    incr_ = -1;
  } else {
    incr_ = 1;
  }
  velocity = ev.data2;
  eg.StartEg();
  stop_pending_ = false;
}

void DrumSampler::NoteOff(midi_event ev) {
  eg.Release();
  stop_pending_ = true;
}

void DrumSampler::PitchBend(midi_event ev) {
  float pitch_val = ev.data1 / 10.;
  SetPitch(pitch_val);
}

bool DrumSampler::ImportFile(std::string filename) {
  file_buffer_ = std::make_unique<FileBuffer>(filename);
  if (file_buffer_) return true;
  return false;
}

void DrumSampler::SetPitch(double v) {
  file_buffer_->SetPitch(v);
}

void DrumSampler::SetAttackTime(double val) {
  eg.SetAttackTimeMsec(val);
}
void DrumSampler::SetDecayTime(double val) {
  eg.SetDecayTimeMsec(val);
}
void DrumSampler::SetSustainLvl(double val) {
  eg.SetSustainLevel(val);
}
void DrumSampler::SetReleaseTime(double val) {
  eg.SetReleaseTimeMsec(val);
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
  else if (name == "reverse") {
    reverse_mode = val;
    incr_ = -1 * reverse_mode;
  }
}

}  // namespace SBAudio
