#pragma once

#include <fftw3.h>

#include <array>

namespace {
const int kFFTSize = 1024;
const int kHopSize = 256;
const int kBufferSize = kFFTSize * 16;
}  // namespace

struct FFTProcessor {
  FFTProcessor();
  ~FFTProcessor();

  double ProcessData(double val);
  void ProcessFFT();

  int fft_hop_count_{0};

  // input ring buffer
  std::array<double, kBufferSize> buffer_in_;
  int buffer_in_write_idx_{0};

  // output ring buffer
  std::array<double, kBufferSize> buffer_out_;
  int buffer_out_read_idx_{0};
  int buffer_out_write_idx_{kHopSize};

  // Fwd FFT
  fftw_plan fft_fwd_plan_;
  std::array<double, kFFTSize> fft_double_in_;
  fftw_complex fft_complex_out_[kFFTSize / 2 + 1];

  // Inverse FFT
  fftw_plan fft_rvr_plan_;
  std::array<double, kFFTSize> fft_double_out_;

  // pitch shifty
  int max_bin_index_{0};  // bin with peak magnitude
  double max_bin_value_{0};

  std::array<double, kFFTSize> last_input_phases_;
  std::array<double, kFFTSize / 2 + 1> analysis_magnitudes_;
  std::array<double, kFFTSize / 2 + 1> analysis_frequencies_;

  std::array<double, kFFTSize / 2 + 1> center_frequencies_;

  std::array<double, kFFTSize> hann_window_buffer_;
};
