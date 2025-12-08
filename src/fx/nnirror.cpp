#include <fx/nnirror.h>

Nnirror::Nnirror() {
  for (int i = 0; i < 12; i++) {
    float delay_ms = 2.0f + i * 1.5f;  // 2ms to 18.5ms
    delay_taps_.push_back({.delay_samples = MsToSamples(delay_ms),
                           .feedback = 0.3f / (i + 1),
                           .cross_feed = 0.2f});
  }

  kernel_ = {{{0.0625f, 0.125f, 0.0625f},
              {0.125f, 0.25f, 0.125f},
              {0.0625f, 0.125f, 0.0625f}}};

  delay_buffers_[0].resize(kMaxDelaySamples, 0.0f);
  delay_buffers_[1].resize(kMaxDelaySamples, 0.0f);

  type_ = fx_type::NNIRROR;
  enabled_ = true;
}

std::string Nnirror::Status() {
  return "";
}
StereoVal Nnirror::Process(StereoVal input) {
  delay_buffers_[0][write_indices_[0]] = input.left;
  delay_buffers_[1][write_indices_[1]] = input.right;

  float blurred_left = 0.0f;
  float blurred_right = 0.0f;

  // Apply 2D blur across time and channels
  for (int t = -1; t <= 1; t++) {    // Time dimension
    for (int c = -1; c <= 1; c++) {  // Channel dimension
      // Calculate sample indices with wrap-around
      int left_idx = WrapIndex(write_indices_[0] - t);
      int right_idx = WrapIndex(write_indices_[1] - t);

      // Get kernel weight
      float weight = kernel_[t + 1][c + 1];

      // Apply blur with channel crossing
      if (c <= 0) {
        blurred_left += delay_buffers_[0][left_idx] * weight;
      }
      if (c >= 0) {
        blurred_right += delay_buffers_[1][right_idx] * weight;
      }
    }
  }

  // Mix in delay taps with feedback
  for (const auto& tap : delay_taps_) {
    int left_tap_idx = WrapIndex(write_indices_[0] - tap.delay_samples);
    int right_tap_idx = WrapIndex(write_indices_[1] - tap.delay_samples);

    float left_delayed = delay_buffers_[0][left_tap_idx];
    float right_delayed = delay_buffers_[1][right_tap_idx];

    // Add delayed signal with cross-feeding
    blurred_left +=
        left_delayed * tap.feedback + right_delayed * tap.cross_feed;
    blurred_right +=
        right_delayed * tap.feedback + left_delayed * tap.cross_feed;
  }

  // Increment write indices
  write_indices_[0] = WrapIndex(write_indices_[0] + 1);
  write_indices_[1] = WrapIndex(write_indices_[1] + 1);

  StereoVal output;
  // Mix with dry signal
  output.left = input.left * 0.5f + blurred_left * 0.5f;
  output.right = input.right * 0.5f + blurred_right * 0.5f;
  return output;
}

void Nnirror::SetParam(std::string name, double val) {}

void Nnirror::EventNotify(broadcast_event event, mixer_timing_info tinfo) {}
