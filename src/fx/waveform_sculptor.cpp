#include "waveform_sculptor.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

#include "defjams.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
}  // namespace

WaveformSculptor::WaveformSculptor() {
  type_ = fx_type::GEOMETER;

  // Default parameters
  window_size_ = 1024;
  window_shape_ = WindowShape::kCosine;
  landmark_style_ = LandmarkStyle::kExtNCross;
  landmark_param_ = 0.0;
  interp_style_ = InterpStyle::kPolygon;
  interp_param_ = 0.0;
  point_op_ = PointOp::kNone;
  op_param_ = 0.5;
  wet_mix_ = 1.0;  // 100% wet by default

  last_num_landmarks_ = 0;

  // Initialize working buffers (silence cppcheck warnings)
  temp_x_.fill(0);
  temp_y_.fill(0.0f);

  Init();
  enabled_ = true;
}

void WaveformSculptor::Init() {
  hop_size_ = window_size_ / 2;  // 50% overlap

  // Allocate buffers
  input_buffer_.resize(window_size_, 0.0f);
  output_buffer_.resize(window_size_, 0.0f);
  last_window_.resize(window_size_, 0.0f);

  Reset();
}

void WaveformSculptor::Reset() {
  std::fill(input_buffer_.begin(), input_buffer_.end(), 0.0f);
  std::fill(output_buffer_.begin(), output_buffer_.end(), 0.0f);
  std::fill(last_window_.begin(), last_window_.end(), 0.0f);
  samples_in_buffer_ = 0;
  output_position_ = 0;
}

void WaveformSculptor::SetInterpDefaults(InterpStyle style) {
  switch (style) {
    case InterpStyle::kPolygon:
      // Lo-fi geometric sound - regular spacing, some dimming
      landmark_style_ = LandmarkStyle::kFreq;
      landmark_param_ = 0.08;  // ~12-15 landmarks for obvious polygons
      interp_param_ = 0.3;     // Light dimming
      break;

    case InterpStyle::kWrongygon:
      // Harsh backwards lines - sparse for dramatic effect
      landmark_style_ = LandmarkStyle::kFreq;
      landmark_param_ = 0.05;  // ~20 landmarks
      interp_param_ = 0.0;     // No dimming
      break;

    case InterpStyle::kSing:
      // Musical tones - very sparse for lower pitches
      landmark_style_ = LandmarkStyle::kFreq;
      landmark_param_ = 0.015;  // ~15-20 landmarks = musical pitches
      interp_param_ = 0.5;      // Mix sine with original (less extreme)
      break;

    case InterpStyle::kReversi:
      // Reverse intervals - moderate landmarks
      landmark_style_ = LandmarkStyle::kExtNCross;
      landmark_param_ = 0.3;  // Threshold to reduce landmarks
      interp_param_ = 0.5;    // Doesn't affect reversi much
      break;

    case InterpStyle::kSmoothie:
      // Smooth curves - moderate spacing
      landmark_style_ = LandmarkStyle::kExtNCross;
      landmark_param_ = 0.2;
      interp_param_ = 0.5;
      break;

    case InterpStyle::kPulse:
      // Pulses - sparse landmarks
      landmark_style_ = LandmarkStyle::kFreq;
      landmark_param_ = 0.05;
      interp_param_ = 0.8;
      break;
  }
}

float WaveformSculptor::GetWindowScalar(int position, int window_size) {
  int half = window_size / 2;

  switch (window_shape_) {
    case WindowShape::kTriangle:
      // Triangle: rises from 0 to 1 in first half, falls from 1 to 0 in second
      // half
      if (position < half) {
        return static_cast<float>(position) / static_cast<float>(half);
      } else {
        return 1.0f -
               (static_cast<float>(position - half) / static_cast<float>(half));
      }

    case WindowShape::kCosine:
    default: {
      // Cosine window: smooth version of triangle
      float t =
          static_cast<float>(position) / static_cast<float>(window_size - 1);
      return 0.5f * (1.0f - std::cos(kTwoPi * t));
    }
  }
}

StereoVal WaveformSculptor::Process(StereoVal input) {
  if (!enabled_) {
    return input;
  }

  // Convert stereo to mono for processing
  float mono_input = static_cast<float>((input.left + input.right) * 0.5);

  // Add to input buffer
  input_buffer_[samples_in_buffer_] = mono_input;
  samples_in_buffer_++;

  StereoVal output;

  // When we have a full window, process it
  if (samples_in_buffer_ >= window_size_) {
    ProcessWindow();

    // Shift input buffer by hop_size for 50% overlap
    std::copy(input_buffer_.begin() + hop_size_, input_buffer_.end(),
              input_buffer_.begin());
    samples_in_buffer_ -= hop_size_;
    output_position_ = 0;
  }

  // Output from processed buffer (or passthrough initially)
  if (samples_in_buffer_ >= hop_size_) {
    // We have processed audio available
    float processed_output = output_buffer_[output_position_];

    // Wet/dry mix
    float wet = processed_output;
    float dry_mono = mono_input;
    float mixed = dry_mono * static_cast<float>(1.0 - wet_mix_) +
                  wet * static_cast<float>(wet_mix_);

    output.left = mixed;
    output.right = mixed;
    output_position_++;
  } else {
    // Initial fill - pass through
    output = input;
  }

  return output;
}

void WaveformSculptor::ProcessWindow() {
  // Step 1: Generate landmarks
  int num_points =
      GenerateLandmarks(input_buffer_.data(), window_size_, temp_x_.data(),
                        temp_y_.data(), kMaxLandmarks);

  last_num_landmarks_ = num_points;

  // Step 2: Apply operation on points
  ApplyOperation(temp_x_.data(), temp_y_.data(), num_points, point_op_,
                 op_param_);

  // Step 3: Recreate waveform
  std::vector<float> recreated(window_size_, 0.0f);
  RecreateWaveform(temp_x_.data(), temp_y_.data(), num_points,
                   input_buffer_.data(), recreated.data(), window_size_);

  // Step 4: Apply window envelope to recreated signal
  for (int i = 0; i < window_size_; i++) {
    float window_scalar = GetWindowScalar(i, window_size_);
    recreated[i] *= window_scalar;
  }

  // Step 5: Overlap-add: Add second half of previous window to first half of
  // current
  for (int i = 0; i < hop_size_; i++) {
    output_buffer_[i] = recreated[i] + last_window_[hop_size_ + i];
  }

  // Second half of current window becomes first half of output
  for (int i = 0; i < hop_size_; i++) {
    output_buffer_[hop_size_ + i] = recreated[hop_size_ + i];
  }

  // Save this window for next overlap-add
  std::copy(recreated.begin(), recreated.end(), last_window_.begin());
}

int WaveformSculptor::GenerateLandmarks(const float* input, int size,
                                        int* out_x, float* out_y,
                                        int max_points) {
  switch (landmark_style_) {
    case LandmarkStyle::kExtNCross:
      return GenerateExtNCross(input, size, out_x, out_y, max_points);
    case LandmarkStyle::kSpan:
      return GenerateSpan(input, size, out_x, out_y, max_points);
    case LandmarkStyle::kDyDx:
      return GenerateDyDx(input, size, out_x, out_y, max_points);
    case LandmarkStyle::kFreq: {
      // Regular frequency spacing
      float freq = 0.02f + (landmark_param_ * 0.2f);  // 0.02 to 0.22
      int num = 0;
      for (float x = 0; x < size && num < max_points; x += 1.0f / freq) {
        int ix = static_cast<int>(x);
        if (ix < size) {
          out_x[num] = ix;
          out_y[num] = input[ix];
          num++;
        }
      }
      return num;
    }
    case LandmarkStyle::kRandom: {
      // Random points
      int num_random = static_cast<int>(landmark_param_ * max_points * 0.5f);
      for (int i = 0; i < num_random && i < max_points; i++) {
        out_x[i] = rand() % size;
        out_y[i] = input[out_x[i]];
      }
      return num_random;
    }
    default:
      return 0;
  }
}

void WaveformSculptor::ApplyOperation(int* x_points, float* y_points,
                                      int& num_points, PointOp op,
                                      double param) {
  if (op == PointOp::kNone || num_points == 0) return;

  switch (op) {
    case PointOp::kHalf: {
      // Remove every other point
      int new_count = 0;
      for (int i = 0; i < num_points; i += 2) {
        x_points[new_count] = x_points[i];
        y_points[new_count] = y_points[i];
        new_count++;
      }
      num_points = new_count;
      break;
    }

    case PointOp::kLongpass: {
      // Remove points that are too close together
      float min_distance = param * 100.0f;  // 0 to 100 samples
      int new_count = 0;
      int last_x = -1000;

      for (int i = 0; i < num_points; i++) {
        if (x_points[i] - last_x >= min_distance) {
          x_points[new_count] = x_points[i];
          y_points[new_count] = y_points[i];
          last_x = x_points[i];
          new_count++;
        }
      }
      num_points = new_count;
      break;
    }

    case PointOp::kSlow: {
      // Stretch points (time stretch)
      float factor = 1.0f + (param * 3.0f);  // 1.0 to 4.0
      for (int i = 0; i < num_points; i++) {
        x_points[i] = static_cast<int>(x_points[i] * factor);
      }
      break;
    }

    case PointOp::kFast: {
      // Compress points (time compress)
      float factor = 1.0f / (1.0f + (param * 3.0f));  // 1.0 to 0.25
      for (int i = 0; i < num_points; i++) {
        x_points[i] = static_cast<int>(x_points[i] * factor);
      }
      break;
    }

    case PointOp::kDouble: {
      // Insert points between existing ones
      int original_count = num_points;
      int new_count = 0;
      for (int i = 0; i < original_count - 1 && new_count < kMaxLandmarks - 2;
           i++) {
        // Original point
        temp_x_[new_count] = x_points[i];
        temp_y_[new_count] = y_points[i];
        new_count++;

        // Interpolated point
        int mid_x = (x_points[i] + x_points[i + 1]) / 2;
        float mid_y =
            (y_points[i] + y_points[i + 1]) * 0.5f * static_cast<float>(param);
        temp_x_[new_count] = mid_x;
        temp_y_[new_count] = mid_y;
        new_count++;
      }
      // Last point
      if (new_count < kMaxLandmarks) {
        temp_x_[new_count] = x_points[original_count - 1];
        temp_y_[new_count] = y_points[original_count - 1];
        new_count++;
      }

      // Copy back
      for (int i = 0; i < new_count; i++) {
        x_points[i] = temp_x_[i];
        y_points[i] = temp_y_[i];
      }
      num_points = new_count;
      break;
    }

    default:
      break;
  }
}

void WaveformSculptor::RecreateWaveform(const int* x_points,
                                        const float* y_points, int num_points,
                                        const float* input, float* output,
                                        int size) {
  if (num_points == 0) {
    // No points, output silence
    std::fill(output, output + size, 0.0f);
    return;
  }

  switch (interp_style_) {
    case InterpStyle::kPolygon:
      RecreatePolygon(x_points, y_points, num_points, output, size);
      break;
    case InterpStyle::kWrongygon:
      RecreateWrongygon(x_points, y_points, num_points, output, size);
      break;
    case InterpStyle::kSing:
      RecreateSing(x_points, y_points, num_points, input, output, size);
      break;
    case InterpStyle::kReversi:
      RecreateReversi(x_points, y_points, num_points, input, output, size);
      break;
    default:
      // Default to polygon
      RecreatePolygon(x_points, y_points, num_points, output, size);
      break;
  }
}

// Landmark generation implementations

int WaveformSculptor::GenerateExtNCross(const float* input, int size,
                                        int* out_x, float* out_y,
                                        int max_points) {
  // Detect extremities (peaks/valleys) and zero crossings
  float threshold = landmark_param_ * 0.1f;  // Noise threshold
  int num = 0;

  // Always include first point
  out_x[num] = 0;
  out_y[num] = input[0];
  num++;

  bool was_positive = input[0] > 0;

  for (int i = 1; i < size - 1 && num < max_points - 1; i++) {
    bool is_positive = input[i] > threshold;
    bool is_negative = input[i] < -threshold;

    // Zero crossing
    if ((was_positive && is_negative) || (!was_positive && is_positive)) {
      out_x[num] = i;
      out_y[num] = input[i];
      num++;
      was_positive = is_positive;
    }
    // Local maximum
    else if (input[i] > input[i - 1] && input[i] > input[i + 1] &&
             std::abs(input[i]) > threshold) {
      out_x[num] = i;
      out_y[num] = input[i];
      num++;
    }
    // Local minimum
    else if (input[i] < input[i - 1] && input[i] < input[i + 1] &&
             std::abs(input[i]) > threshold) {
      out_x[num] = i;
      out_y[num] = input[i];
      num++;
    }
  }

  // Always include last point
  if (num < max_points) {
    out_x[num] = size - 1;
    out_y[num] = input[size - 1];
    num++;
  }

  return num;
}

int WaveformSculptor::GenerateSpan(const float* input, int size, int* out_x,
                                   float* out_y, int max_points) {
  // Spacing based on amplitude
  float influence = 10.0f + (landmark_param_ * 100.0f);  // How much amplitude
                                                         // affects spacing
  int num = 0;
  int x = 0;

  while (x < size && num < max_points - 1) {
    out_x[num] = x;
    out_y[num] = input[x];

    // Next point distance based on amplitude
    float amp = std::abs(input[x]);
    int distance = static_cast<int>(influence * (1.0f - amp)) + 1;
    x += distance;
    num++;
  }

  // Always include last point
  if (num < max_points && (num == 0 || out_x[num - 1] != size - 1)) {
    out_x[num] = size - 1;
    out_y[num] = input[size - 1];
    num++;
  }

  return num;
}

int WaveformSculptor::GenerateDyDx(const float* input, int size, int* out_x,
                                   float* out_y, int max_points) {
  // Based on derivative (rate of change)
  float threshold = (landmark_param_ - 0.5f) * 2.0f;  // -1 to 1
  int num = 0;

  // Always include first point
  out_x[num] = 0;
  out_y[num] = input[0];
  num++;

  for (int i = 1; i < size && num < max_points - 1; i++) {
    float derivative = input[i] - input[i - 1];
    if (std::abs(derivative) > std::abs(threshold)) {
      out_x[num] = i;
      out_y[num] = input[i];
      num++;
    }
  }

  // Always include last point
  if (num < max_points) {
    out_x[num] = size - 1;
    out_y[num] = input[size - 1];
    num++;
  }

  return num;
}

// Waveform recreation implementations

void WaveformSculptor::RecreatePolygon(const int* x_points,
                                       const float* y_points, int num_points,
                                       float* output, int size) {
  // Linear interpolation with dimming effect - matches original DestroyFX
  for (int i = 1; i < num_points; i++) {
    int x1 = x_points[i - 1];
    int x2 = x_points[i];
    float y1 = y_points[i - 1];
    float y2 = y_points[i];

    if (x2 <= x1 || x1 >= size) continue;

    float const denom = static_cast<float>(x2 - x1);
    float const median = static_cast<float>(interp_param_) * (y1 + y2) * 0.5f;

    for (int x = x1; x < x2 && x < size; x++) {
      float const pct = static_cast<float>(x - x1) / denom;
      float const s = y1 * (1.0f - pct) + y2 * pct;
      output[x] = median + (1.0f - static_cast<float>(interp_param_)) * s;
    }
  }

  output[size - 1] = y_points[num_points - 1];
}

void WaveformSculptor::RecreateWrongygon(const int* x_points,
                                         const float* y_points, int num_points,
                                         float* output, int size) {
  // Backwards linear interpolation - harsh/glitchy effect
  for (int i = 1; i < num_points; i++) {
    int x1 = x_points[i - 1];
    int x2 = x_points[i];
    float y1 = y_points[i - 1];
    float y2 = y_points[i];

    if (x2 <= x1 || x1 >= size) continue;

    float const denom = static_cast<float>(x2 - x1);
    float const median = static_cast<float>(interp_param_) * (y1 + y2) * 0.5f;

    for (int x = x1; x < x2 && x < size; x++) {
      float const pct = static_cast<float>(x - x1) / denom;
      // Backwards interpolation: y1 * pct + y2 * (1 - pct)
      float const s = y1 * pct + y2 * (1.0f - pct);
      output[x] = median + (1.0f - static_cast<float>(interp_param_)) * s;
    }
  }

  output[size - 1] = y_points[num_points - 1];
}

void WaveformSculptor::RecreateSing(const int* x_points, const float* y_points,
                                    int num_points, const float* input,
                                    float* output, int size) {
  // Sine wave interpolation with amplitude envelope from landmarks
  for (int i = 1; i < num_points; i++) {
    int x1 = x_points[i - 1];
    int x2 = x_points[i];
    float y1 = y_points[i - 1];
    float y2 = y_points[i];

    if (x2 <= x1 || x1 >= size) continue;

    float const oodenom = 1.0f / static_cast<float>(x2 - x1);

    for (int x = x1; x < x2 && x < size; x++) {
      float const pct = static_cast<float>(x - x1) * oodenom;

      // Generate one sine cycle per interval
      float const wand = std::sin(kTwoPi * pct);

      // Interpolate landmark amplitudes to create envelope
      float const amplitude = y1 * (1.0f - pct) + y2 * pct;

      // Mix between pure sine and amplitude modulation
      // Scale entire result by landmark amplitude envelope
      output[x] =
          amplitude *
          (wand * static_cast<float>(interp_param_) +
           ((1.0f - static_cast<float>(interp_param_)) * input[x] * wand));
    }
  }

  output[size - 1] = input[size - 1];
}

void WaveformSculptor::RecreateReversi(const int* x_points,
                                       const float* y_points, int num_points,
                                       const float* input, float* output,
                                       int size) {
  // Reverse the audio between each pair of landmarks
  std::fill(output, output + size, 0.0f);

  for (int i = 0; i < num_points - 1; i++) {
    int x1 = x_points[i];
    int x2 = x_points[i + 1];

    if (x2 <= x1 || x1 >= size) continue;

    // Copy interval in reverse
    for (int x = x1; x < x2 && x < size; x++) {
      int reverse_x = x2 - (x - x1) - 1;
      if (reverse_x >= 0 && reverse_x < size) {
        output[x] = input[reverse_x];
      }
    }
  }

  // Fill any gaps with last value
  if (num_points > 0) {
    int last_x = x_points[num_points - 1];
    if (last_x >= 0 && last_x < size) {
      float last_y = input[last_x];
      for (int x = last_x; x < size; x++) {
        output[x] = last_y;
      }
    }
  }
}

std::string WaveformSculptor::Status() {
  std::ostringstream oss;
  oss << "WaveformSculptor!";
  oss << " wsize:" << window_size_;
  oss << " [" << last_num_landmarks_ << " pts]";

  // Landmark style
  oss << " landmarks:";
  switch (landmark_style_) {
    case LandmarkStyle::kExtNCross:
      oss << "ext'n'cross";
      break;
    case LandmarkStyle::kSpan:
      oss << "span";
      break;
    case LandmarkStyle::kDyDx:
      oss << "dy/dx";
      break;
    case LandmarkStyle::kFreq:
      oss << "freq";
      break;
    case LandmarkStyle::kRandom:
      oss << "random";
      break;
  }
  oss << "\nlparam:" << landmark_param_;

  // Interpolation style
  oss << " interp:";
  switch (interp_style_) {
    case InterpStyle::kPolygon:
      oss << "polygon";
      break;
    case InterpStyle::kSing:
      oss << "sing";
      break;
    case InterpStyle::kReversi:
      oss << "reversi";
      break;
    case InterpStyle::kSmoothie:
      oss << "smoothie";
      break;
    case InterpStyle::kPulse:
      oss << "pulse";
      break;
    case InterpStyle::kWrongygon:
      oss << "wrongygon";
      break;
  }
  oss << " iparam:" << interp_param_;

  // Operation
  oss << " op:";
  switch (point_op_) {
    case PointOp::kNone:
      oss << "none";
      break;
    case PointOp::kDouble:
      oss << "double";
      break;
    case PointOp::kHalf:
      oss << "half";
      break;
    case PointOp::kLongpass:
      oss << "longpass";
      break;
    case PointOp::kSlow:
      oss << "slow";
      break;
    case PointOp::kFast:
      oss << "fast";
      break;
  }
  oss << " oparam:" << op_param_;
  oss << " mix:" << wet_mix_;

  return oss.str();
}

void WaveformSculptor::SetParam(std::string name, double val) {
  if (name == "wsize") {
    int size = static_cast<int>(val);
    if (size >= kMinWindowSize && size <= kMaxWindowSize) {
      window_size_ = size;
      Init();  // Reinitialize buffers
    }
  } else if (name == "landmarks") {
    int style = static_cast<int>(val);
    if (style >= 0 && style <= 4) {
      landmark_style_ = static_cast<LandmarkStyle>(style);
    }
  } else if (name == "landmark_param" || name == "lparam") {
    landmark_param_ = std::max(0.0, std::min(1.0, val));
  } else if (name == "interp") {
    int style = static_cast<int>(val);
    if (style >= 0 && style <= 5) {
      interp_style_ = static_cast<InterpStyle>(style);
      SetInterpDefaults(interp_style_);  // Auto-set good defaults
    }
  } else if (name == "interp_param" || name == "iparam") {
    interp_param_ = std::max(0.0, std::min(1.0, val));
  } else if (name == "op" || name == "pointop") {
    int op = static_cast<int>(val);
    if (op >= 0 && op <= 5) {
      point_op_ = static_cast<PointOp>(op);
    }
  } else if (name == "op_param" || name == "oparam") {
    op_param_ = std::max(0.0, std::min(1.0, val));
  } else if (name == "mix" || name == "wet") {
    wet_mix_ = std::max(0.0, std::min(1.0, val));
  } else if (name == "enabled") {
    enabled_ = val > 0.5;
  }
}
