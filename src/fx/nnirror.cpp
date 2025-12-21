#include <fx/nnirror.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

// DelayLine implementation
Nnirror::DelayLine::DelayLine(int max_samples)
    : max_samples_(max_samples), buffer_(max_samples, 0.0f) {}

void Nnirror::DelayLine::Write(double sample) {
  buffer_[write_index_] = sample;
  write_index_ = (write_index_ + 1) % max_samples_;
}

double Nnirror::DelayLine::Read(int delay_samples) const {
  int read_index = (write_index_ - delay_samples + max_samples_) % max_samples_;
  return buffer_[read_index];
}

double Nnirror::DelayLine::ReadInterpolated(double delay_samples) const {
  int delay_int = static_cast<int>(delay_samples);
  double frac = delay_samples - delay_int;

  double sample1 = Read(delay_int);
  double sample2 = Read(delay_int + 1);

  return sample1 + frac * (sample2 - sample1);
}

void Nnirror::DelayLine::Clear() {
  std::fill(buffer_.begin(), buffer_.end(), 0.0f);
  write_index_ = 0;
}

// AllPassFilter implementation
Nnirror::AllPassFilter::AllPassFilter(int delay_samples, double feedback)
    : delay_(delay_samples * 2),
      feedback_(feedback),
      delay_samples_(delay_samples) {}

double Nnirror::AllPassFilter::Process(double input) {
  double delayed = delay_.Read(delay_samples_);
  double output = -input + delayed;
  delay_.Write(input + delayed * feedback_);
  return output;
}

void Nnirror::AllPassFilter::Clear() {
  delay_.Clear();
}

// DiffusionStage implementation
Nnirror::DiffusionStage::DiffusionStage(int sample_rate) {
  // Prime number delay times for decorrelation
  const int prime_delays_ms[kNumAllPasses] = {37, 43, 53, 61};
  const double feedback = 0.5f;

  // Reserve space and construct with emplace_back
  left_apfs_.reserve(kNumAllPasses);
  right_apfs_.reserve(kNumAllPasses);

  for (int i = 0; i < kNumAllPasses; ++i) {
    int delay_samples = (prime_delays_ms[i] * sample_rate) / 1000;
    left_apfs_[i] = AllPassFilter(delay_samples, feedback);
    right_apfs_[i] =
        AllPassFilter(delay_samples + 7, feedback);  // Slight offset
  }
}

StereoVal Nnirror::DiffusionStage::Process(StereoVal input, double amount) {
  if (amount < 0.001) return input;

  StereoVal wet = input;

  for (int i = 0; i < kNumAllPasses; ++i) {
    wet.left = left_apfs_[i].Process(wet.left);
    wet.right = right_apfs_[i].Process(wet.right);

    // Cross-coupling for stereo width
    double cross = amount * 0.3;
    double temp = wet.left;
    wet.left = wet.left * (1.0 - cross) + wet.right * cross;
    wet.right = wet.right * (1.0 - cross) + temp * cross;
  }

  return {input.left * (1.0 - amount) + wet.left * amount,
          input.right * (1.0 - amount) + wet.right * amount};
}

void Nnirror::DiffusionStage::Clear() {
  for (auto& apf : left_apfs_) apf.Clear();
  for (auto& apf : right_apfs_) apf.Clear();
}

// SmoothParameter implementation
void Nnirror::SmoothParameter::SetInertia(double inertia) {
  // 0 = instant, 1 = very slow
  double time_constant = 0.001f + inertia * inertia * 2.0f;
  coefficient_ = 1.0f - std::exp(-1.0f / (time_constant * 44100.0f));
}

double Nnirror::SmoothParameter::Process() {
  current_ += coefficient_ * (target_ - current_);
  return current_;
}

void Nnirror::SmoothParameter::Reset(double value) {
  current_ = value;
  target_ = value;
}

Nnirror::Nnirror() : max_delay_samples_(SAMPLE_RATE * kMaxDelayMs / 1000) {
  // Initialize delay lines
  main_delays_[0] = std::make_unique<DelayLine>(max_delay_samples_);
  main_delays_[1] = std::make_unique<DelayLine>(max_delay_samples_);
  feedback_delays_[0] = std::make_unique<DelayLine>(max_delay_samples_);
  feedback_delays_[1] = std::make_unique<DelayLine>(max_delay_samples_);

  // Initialize diffusion stages
  for (int i = 0; i < 4; ++i) {
    diffusion_stages_[i] = std::make_unique<DiffusionStage>(SAMPLE_RATE);
  }

  // Initialize kernel
  kernel_size_ = 3;
  kernel_.resize(kernel_size_, std::vector<double>(kernel_size_));

  // Gaussian kernel
  kernel_ = {
      {0.0625, 0.125, 0.0625}, {0.125, 0.25, 0.125}, {0.0625, 0.125, 0.0625}};

  UpdateParameters();

  // Reset smoothing
  smooth_wet_.Reset(params_.wet);
  smooth_wet_gain_.Reset(params_.wet_gain);
  smooth_size_.Reset(params_.size);
  smooth_feedback_.Reset(params_.feedback);
  smooth_unison_.Reset(params_.unison);

  for (int i = 0; i < 4; ++i) {
    smooth_diffuse_[i].Reset(params_.diffuse[i]);
    inertia_processors_[i].SetInertia(params_.inertia[i]);
  }

  type_ = fx_type::NNIRROR;
  enabled_ = true;
}

std::string Nnirror::Status() {
  std::stringstream ss;
  ss << "Nnirror! ";
  ss << "wet:" << smooth_wet_.GetCurrent();
  ss << " wet_gain:" << smooth_wet_gain_.GetCurrent();
  ss << " sz:" << smooth_size_.GetCurrent();
  ss << " fb:" << smooth_feedback_.GetCurrent();
  ss << " uni:" << smooth_unison_.GetCurrent() << "\n";
  ss << "dif2:" << smooth_diffuse_[1].GetCurrent();
  ss << " dif3:" << smooth_diffuse_[2].GetCurrent();
  ss << " dif4:" << smooth_diffuse_[3].GetCurrent();
  ss << " inert1:" << inertia_processors_[0].GetCurrent();
  ss << " inert2:" << inertia_processors_[1].GetCurrent();
  ss << " inert3:" << inertia_processors_[2].GetCurrent();
  ss << " inert4:" << inertia_processors_[3].GetCurrent();

  return ss.str();
}

StereoVal Nnirror::Process(StereoVal input) {
  // Smooth parameters with inertia
  double wet = smooth_wet_.Process();
  double wet_gain = smooth_wet_gain_.Process();
  double size = smooth_size_.Process();
  double feedback = smooth_feedback_.Process();

  // Update size-dependent parameters if changed significantly
  static double last_size = 0.0;
  if (std::abs(size - last_size) > 0.01) {
    UpdateSize(size);
    last_size = size;
  }

  // Store dry signal
  StereoVal dry = input;

  // Apply feedback
  input = ApplyFeedback(input);

  // Process blur
  StereoVal blurred = ProcessBlur(input);

  // Process unison if active
  if (smooth_unison_.GetCurrent() > 0.01) {
    blurred = ProcessUnison(blurred);
  }

  // Process diffusion stages
  blurred = ProcessDiffusion(blurred);

  // Store for feedback
  feedback_buffer_.left = blurred.left * feedback;
  feedback_buffer_.right = blurred.right * feedback;

  // Apply wet gain
  blurred.left *= wet_gain;
  blurred.right *= wet_gain;

  // Final dry/wet mix
  StereoVal output = {dry.left * (1.0 - wet) + blurred.left * wet,
                      dry.right * (1.0 - wet) + blurred.right * wet};
  output.left = std::max(-1.0, std::min(1.0, output.left));
  output.right = std::max(-1.0, std::min(1.0, output.right));

  if (std::abs(output.left) > 0.95 || std::abs(output.right) > 0.95) {
    feedback_buffer_.left *= 0.5;  // Emergency damping
    feedback_buffer_.right *= 0.5;
  }

  return output;
}

StereoVal Nnirror::ProcessBlur(StereoVal input) {
  // Write to delay lines
  main_delays_[0]->Write(input.left);
  main_delays_[1]->Write(input.right);

  StereoVal output = {0.0, 0.0};

  // Apply 2D kernel blur
  int half_kernel = kernel_size_ / 2;

  for (int t = -half_kernel; t <= half_kernel; ++t) {
    for (int c = -half_kernel; c <= half_kernel; ++c) {
      int kernel_x = t + half_kernel;
      int kernel_y = c + half_kernel;

      if (kernel_x >= kernel_.size() || kernel_y >= kernel_[0].size()) continue;

      double weight = kernel_[kernel_x][kernel_y];

      // Time dimension: read from delays
      int delay_samples = std::abs(t) * 10;

      // Channel dimension: cross-feed
      if (c <= 0) {
        output.left +=
            main_delays_[0]->Read(delay_samples) * weight * (1.0 + c * 0.5);
      }
      if (c >= 0) {
        output.right +=
            main_delays_[1]->Read(delay_samples) * weight * (1.0 - c * 0.5);
      }
    }
  }

  // Add delay taps
  for (size_t i = 0; i < active_taps_ && i < delay_taps_.size(); ++i) {
    const auto& tap = delay_taps_[i];

    // Add modulation to delay time
    double mod = std::sin(lfo_phase_ + i * 0.5) * 2.0;
    double modulated_delay = tap.delay_samples + mod;

    double tap_left = main_delays_[0]->ReadInterpolated(modulated_delay);
    double tap_right = main_delays_[1]->ReadInterpolated(modulated_delay);

    // Apply tap with crossfeed
    output.left += tap_left * tap.feedback + tap_right * tap.cross_feed;
    output.right += tap_right * tap.feedback + tap_left * tap.cross_feed;
  }

  // Update LFO
  lfo_phase_ += lfo_rate_ * 2.0 * M_PI / sample_rate_;
  if (lfo_phase_ > 2.0 * M_PI) lfo_phase_ -= 2.0 * M_PI;

  return output;
}

StereoVal Nnirror::ProcessUnison(StereoVal input) {
  if (unison_voices_.empty()) return input;

  StereoVal unison = {0.0, 0.0};

  for (size_t i = 0; i < unison_voices_.size(); ++i) {
    const auto& voice = unison_voices_[i];

    // Modulated delay for detuning effect
    voice.phase += voice.detune_ratio * 0.01;
    double delay_mod = std::sin(voice.phase) * 3.0 + voice.delay_offset;

    // Read from feedback delays for unison
    double voice_left = feedback_delays_[0]->ReadInterpolated(delay_mod);
    double voice_right = feedback_delays_[1]->ReadInterpolated(delay_mod);

    // Apply panning
    unison.left +=
        voice_left * voice.pan_left + voice_right * (1.0 - voice.pan_right);
    unison.right +=
        voice_right * voice.pan_right + voice_left * (1.0 - voice.pan_left);
  }

  double unison_mix = smooth_unison_.Process();
  StereoVal output = {input.left * (1.0 - unison_mix) +
                          unison.left * unison_mix / unison_voices_.size(),
                      input.right * (1.0 - unison_mix) +
                          unison.right * unison_mix / unison_voices_.size()};

  // Write to feedback delays
  feedback_delays_[0]->Write(output.left);
  feedback_delays_[1]->Write(output.right);

  return output;
}

StereoVal Nnirror::ProcessDiffusion(StereoVal input) {
  for (int i = 0; i < 4; ++i) {
    double amount = smooth_diffuse_[i].Process();
    if (amount > 0.001) {
      input = diffusion_stages_[i]->Process(input, amount);
    }
  }
  return input;
}

StereoVal Nnirror::ApplyFeedback(StereoVal input) {
  double fb = smooth_feedback_.Process();
  fb = std::min(fb, 0.85);
  return {std::tanh(input.left + feedback_buffer_.left * fb * 0.7),
          std::tanh(input.right + feedback_buffer_.right * fb * 0.7)};
}

void Nnirror::UpdateSize(double size) {
  // Kernel size: 3 to 7
  kernel_size_ = 3 + static_cast<int>(size * 4);
  if (kernel_size_ % 2 == 0) kernel_size_++;

  // Rebuild kernel
  kernel_.clear();
  kernel_.resize(kernel_size_, std::vector<double>(kernel_size_));

  // Generate Gaussian-like kernel
  double sum = 0.0;
  int center = kernel_size_ / 2;
  double sigma = 0.5 + size * 2.0;

  for (int i = 0; i < kernel_size_; ++i) {
    for (int j = 0; j < kernel_size_; ++j) {
      double dist =
          std::sqrt(std::pow(i - center, 2) + std::pow(j - center, 2));
      kernel_[i][j] = std::exp(-dist * dist / (2.0 * sigma * sigma));
      sum += kernel_[i][j];
    }
  }

  // Normalize
  for (auto& row : kernel_) {
    for (auto& val : row) {
      val /= sum;
    }
  }

  // Active taps: 6 to 72
  active_taps_ = 6 + static_cast<int>(size * 66);

  // Rebuild delay taps
  delay_taps_.clear();
  double min_delay = 0.5 + (1.0 - size) * 2.0;  // ms
  double max_delay = 20.0 + size * 200.0;       // ms

  for (int i = 0; i < kMaxDelayTaps; ++i) {
    double position = static_cast<double>(i) / kMaxDelayTaps;
    double delay_ms = min_delay + position * (max_delay - min_delay);

    // Use prime-like spacing for better diffusion
    delay_ms *= (1.0 + 0.1 * std::sin(i * 0.618));

    delay_taps_.push_back({.delay_samples = MsToSamples(delay_ms),
                           .feedback = 0.3 / (i + 1),
                           .cross_feed = 0.1 + size * 0.4});
  }
}

void Nnirror::UpdateUnison(double unison) {
  int num_voices = 1 + static_cast<int>(unison * (kMaxUnisonVoices - 1));

  unison_voices_.clear();
  if (num_voices <= 1) return;

  for (int i = 0; i < num_voices; ++i) {
    double position = static_cast<double>(i) / (num_voices - 1);
    double pan = position * 2.0 - 1.0;  // -1 to 1

    unison_voices_.push_back(
        {.detune_ratio = 1.0 + (position - 0.5) * 0.02 * unison,
         .pan_left = std::sqrt(1.0 - pan) * 0.707,
         .pan_right = std::sqrt(1.0 + pan) * 0.707,
         .delay_offset = i * MsToSamples(1.5),
         .phase = i * M_PI / 4.0});
  }
}

void Nnirror::SetParameters(const Parameters& params) {
  target_params_ = params;

  // Update smooth targets
  smooth_wet_.SetTarget(params.wet);
  smooth_wet_gain_.SetTarget(params.wet_gain);
  smooth_size_.SetTarget(params.size);
  smooth_feedback_.SetTarget(params.feedback);
  smooth_unison_.SetTarget(params.unison);

  for (int i = 0; i < 4; ++i) {
    smooth_diffuse_[i].SetTarget(params.diffuse[i]);
    inertia_processors_[i].SetInertia(params.inertia[i]);
  }

  // Set inertia processor targets
  inertia_processors_[0].SetTarget(params.wet);
  inertia_processors_[1].SetTarget(params.size);
  inertia_processors_[2].SetTarget(params.feedback);
  inertia_processors_[3].SetTarget(params.unison);

  UpdateUnison(params.unison);
}

void Nnirror::UpdateParameters() {
  UpdateSize(params_.size);
  UpdateUnison(params_.unison);
}

void Nnirror::Reset() {
  main_delays_[0]->Clear();
  main_delays_[1]->Clear();
  feedback_delays_[0]->Clear();
  feedback_delays_[1]->Clear();

  for (auto& stage : diffusion_stages_) {
    stage->Clear();
  }

  feedback_buffer_ = {0.0, 0.0};
  lfo_phase_ = 0.0;
}

int Nnirror::MsToSamples(double ms) const {
  return static_cast<int>(ms * sample_rate_ / 1000.0);
}

double Nnirror::SamplesToMs(int samples) const {
  return static_cast<double>(samples) * 1000.0 / sample_rate_;
}

int Nnirror::WrapIndex(int index, int buffer_size) const {
  while (index < 0) index += buffer_size;
  return index % buffer_size;
}

void Nnirror::SetParam(std::string name, double val) {
  std::cout << "YO THOR - SET P ! " << name << " " << val << std::endl;
  if (name == "wet") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    smooth_wet_.SetTarget(val);

  } else if (name == "wet_gain") {
    val = std::max(0.0, std::min(2.0, val));  // 0-2 (up to +6dB boost)
    smooth_wet_gain_.SetTarget(val);

  } else if (name == "sz") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    smooth_size_.SetTarget(val);

  } else if (name == "fb") {
    val = std::max(0.0, std::min(0.9, val));  // 0-0.9 (NEVER allow >= 1.0!)
    smooth_feedback_.SetTarget(val);

  } else if (name == "uni") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    smooth_unison_.SetTarget(val);

  } else if (name == "dif1") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    smooth_diffuse_[0].SetTarget(val);

  } else if (name == "dif2") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    smooth_diffuse_[1].SetTarget(val);

  } else if (name == "dif3") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    smooth_diffuse_[2].SetTarget(val);

  } else if (name == "dif4") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    smooth_diffuse_[3].SetTarget(val);

  } else if (name == "inert1") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    inertia_processors_[0].SetInertia(val);

  } else if (name == "inert2") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    inertia_processors_[1].SetInertia(val);

  } else if (name == "inert3") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    inertia_processors_[2].SetInertia(val);

  } else if (name == "inert4") {
    val = std::max(0.0, std::min(1.0, val));  // 0-1
    inertia_processors_[3].SetInertia(val);
  }
}

void Nnirror::EventNotify(broadcast_event event, mixer_timing_info tinfo) {}
