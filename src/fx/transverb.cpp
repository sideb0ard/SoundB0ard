#include "transverb.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

Transverb::Transverb() {
  type_ = fx_type::TRANSVERB;

  // Initialize parameters with defaults
  bsize_param_ = 0.5;  // mid-range buffer size
  speed1_ = 0.6;       // slightly slower playback
  speed2_ = 0.4;       // slower playback
  drymix_ = 1.0;       // 100% dry signal (keeps original level)
  mix1_ = 0.4;         // 40% wet from buffer 1
  mix2_ = 0.4;         // 40% wet from buffer 2
  feed1_ = 0.3;        // moderate feedback
  feed2_ = 0.3;        // moderate feedback
  dist1_ = 0.3;        // left-leaning
  dist2_ = 0.7;        // right-leaning
  freeze_ = false;     // not frozen

  Init();

  enabled_ = true;
}

void Transverb::Init() {
  // Calculate buffer size from parameter (MIN_BUFFER_SIZE to MAX_BUFFER_SIZE)
  bsize_ = MIN_BUFFER_SIZE +
           static_cast<int>((MAX_BUFFER_SIZE - MIN_BUFFER_SIZE) * bsize_param_);

  // Initialize buffers
  buf1_.resize(bsize_);
  buf2_.resize(bsize_);

  Reset();
}

void Transverb::Reset() {
  // Clear buffers
  std::fill(buf1_.begin(), buf1_.end(), 0.0f);
  std::fill(buf2_.begin(), buf2_.end(), 0.0f);

  // Reset positions
  writer_ = 0;
  read1_ = 0.0;
  read2_ = 0.0;

  // Reset smoothing state
  smoothcount1_ = 0;
  smoothcount2_ = 0;
  smoothdur1_ = 0;
  smoothdur2_ = 0;
  smoothstep1_ = 0.0f;
  smoothstep2_ = 0.0f;
  lastr1val_ = 0.0f;
  lastr2val_ = 0.0f;
}

std::string Transverb::Status() {
  std::ostringstream oss;
  oss << "Transverb!";
  oss << " bsize:" << bsize_param_;
  oss << " dry:" << drymix_;
  oss << " speed1:" << speed1_;
  oss << " speed2:" << speed2_ << "\n";
  oss << "mix1:" << mix1_;
  oss << " mix2:" << mix2_;
  oss << " fb1:" << feed1_;
  oss << " fb2:" << feed2_;
  oss << " dist1:" << dist1_;
  oss << " dist2:" << dist2_;
  oss << " freeze:" << (freeze_ ? "ON" : "OFF");
  return oss.str();
}

StereoVal Transverb::Process(StereoVal input) {
  if (!enabled_) {
    return input;
  }

  ProcessParameters();

  // Convert stereo input to mono for processing
  float mono_input = static_cast<float>((input.left + input.right) * 0.5);

  // Read from delay buffers with interpolation
  float r1val = InterpolateHermite(buf1_, read1_);
  float r2val = InterpolateHermite(buf2_, read2_);

  // Apply crossfade smoothing if in progress (prevents clicks)
  if (smoothcount1_ > 0) {
    r1val =
        (r1val * (1.0f - (smoothstep1_ * static_cast<float>(smoothcount1_)))) +
        (lastr1val_ * smoothstep1_ * static_cast<float>(smoothcount1_));
    smoothcount1_--;
  }
  if (smoothcount2_ > 0) {
    r2val =
        (r2val * (1.0f - (smoothstep2_ * static_cast<float>(smoothcount2_)))) +
        (lastr2val_ * smoothstep2_ * static_cast<float>(smoothcount2_));
    smoothcount2_--;
  }

  // Write to buffers with input + feedback (parallel, not cross)
  // Original: each buffer feeds back into itself, feedback scaled by mix
  if (!freeze_) {
    buf1_[writer_] = mono_input + (r1val * static_cast<float>(feed1_ * mix1_));
    buf2_[writer_] = mono_input + (r2val * static_cast<float>(feed2_ * mix2_));
  }
  // If frozen, don't write - keeps reading same buffer content

  // Advance write position (only if not frozen)
  int writer_increment = freeze_ ? 0 : 1;
  writer_ = (writer_ + writer_increment) % bsize_;

  // Store read positions before advancing (for crossing detection)
  int read1int = static_cast<int>(read1_);
  int read2int = static_cast<int>(read2_);

  // Advance read positions
  read1_ += speed1_;
  read2_ += speed2_;

  // Wrap read positions
  if (read1_ >= bsize_) read1_ -= bsize_;
  if (read2_ >= bsize_) read2_ -= bsize_;
  if (read1_ < 0) read1_ += bsize_;

  // Detect read head crossing write head and trigger smoothing
  // This prevents clicks when the read position jumps past the write position
  bool read1_crossing_ahead =
      (read1int < writer_) &&
      (static_cast<int>(read1_ + speed1_) >= (writer_ + 1));
  bool read1_crossing_behind =
      (read1int >= writer_) &&
      (static_cast<int>(read1_ + speed1_) <= (writer_ + 1));
  bool speed1_is_unity = (speed1_ == 1.0);

  if ((read1_crossing_ahead || read1_crossing_behind) && !speed1_is_unity) {
    if (smoothcount1_ <= 0) {
      lastr1val_ = r1val;  // store current value for crossfade
      // truncate smoothing duration if buffer is too small
      double bsize_float = static_cast<double>(bsize_);
      smoothdur1_ = (AUDIO_SMOOTHING_DUR_SAMPLES >
                     static_cast<int>(bsize_float / speed1_))
                        ? static_cast<int>(bsize_float / speed1_)
                        : AUDIO_SMOOTHING_DUR_SAMPLES;
      smoothstep1_ = 1.0f / static_cast<float>(smoothdur1_);
      smoothcount1_ = smoothdur1_;
    }
  }

  // Same for head 2
  bool read2_crossing_ahead =
      (read2int < writer_) &&
      (static_cast<int>(read2_ + speed2_) >= (writer_ + 1));
  bool read2_crossing_behind =
      (read2int >= writer_) &&
      (static_cast<int>(read2_ + speed2_) <= (writer_ + 1));
  bool speed2_is_unity = (speed2_ == 1.0);

  if ((read2_crossing_ahead || read2_crossing_behind) && !speed2_is_unity) {
    if (smoothcount2_ <= 0) {
      lastr2val_ = r2val;
      double bsize_float = static_cast<double>(bsize_);
      smoothdur2_ = (AUDIO_SMOOTHING_DUR_SAMPLES >
                     static_cast<int>(bsize_float / speed2_))
                        ? static_cast<int>(bsize_float / speed2_)
                        : AUDIO_SMOOTHING_DUR_SAMPLES;
      smoothstep2_ = 1.0f / static_cast<float>(smoothdur2_);
      smoothcount2_ = smoothdur2_;
    }
  }

  // Apply wet/dry mixing and stereo distribution
  float wet1_left = r1val * static_cast<float>(mix1_ * (1.0 - dist1_));
  float wet1_right = r1val * static_cast<float>(mix1_ * dist1_);
  float wet2_left = r2val * static_cast<float>(mix2_ * (1.0 - dist2_));
  float wet2_right = r2val * static_cast<float>(mix2_ * dist2_);

  float dry_left = static_cast<float>(input.left * drymix_);
  float dry_right = static_cast<float>(input.right * drymix_);

  StereoVal output;
  output.left = dry_left + wet1_left + wet2_left;
  output.right = dry_right + wet1_right + wet2_right;

  return output;
}

void Transverb::SetParam(std::string name, double val) {
  // Clamp value to 0.0 - 1.0 range
  if (name == "speed1" || name == "speed2") {
    val = std::max(0.25, std::min(4.0, val));
  } else {
    val = std::max(0.0, std::min(1.0, val));
  }

  std::cout << "SET PARAM:" << name << "val" << val << std::endl;

  if (name == "buffer_size") {
    if (bsize_param_ != val) {
      bsize_param_ = val;
      Init();  // Reinitialize with new buffer size
    }
  } else if (name == "speed1") {
    speed1_ = val;
  } else if (name == "speed2") {
    speed2_ = val;
  } else if (name == "dry") {
    drymix_ = val;
  } else if (name == "mix1") {
    mix1_ = val;
  } else if (name == "mix2") {
    mix2_ = val;
  } else if (name == "fb1") {
    feed1_ = val;
  } else if (name == "fb2") {
    feed2_ = val;
  } else if (name == "dist1") {
    dist1_ = val;
  } else if (name == "dist2") {
    dist2_ = val;
  } else if (name == "freeze") {
    freeze_ = val > 0.5;
  } else if (name == "enabled") {
    enabled_ = val > 0.5;
  }
}

void Transverb::ProcessParameters() {
  // This method can be used for parameter smoothing in the future
  // For now, parameters are applied directly
}

float Transverb::InterpolateHermite(const std::vector<float>& data,
                                    double address) {
  int pos = static_cast<int>(address);
  float fract = static_cast<float>(address - pos);

  // Ensure we don't go out of bounds
  pos = pos % static_cast<int>(data.size());
  if (pos < 0) pos += data.size();

  int pos_minus1 = (pos - 1 + data.size()) % data.size();
  int pos_plus1 = (pos + 1) % data.size();
  int pos_plus2 = (pos + 2) % data.size();

  // Hermite interpolation
  float a = ((3.0f * (data[pos] - data[pos_plus1])) - data[pos_minus1] +
             data[pos_plus2]) *
            0.5f;
  float b = (2.0f * data[pos_plus1]) + data[pos_minus1] - (2.5f * data[pos]) -
            (data[pos_plus2] * 0.5f);
  float c = (data[pos_plus1] - data[pos_minus1]) * 0.5f;

  return (((a * fract) + b) * fract + c) * fract + data[pos];
}
