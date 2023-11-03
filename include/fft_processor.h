#pragma once

#include <fftw3.h>

#include <vector>

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

  std::vector<double> buffer_in_;
  int buffer_in_write_idx_{0};

  std::vector<double> buffer_out_;
  int buffer_out_read_idx_{0};
  int buffer_out_write_idx_{kHopSize};

  bool use_fft_{false};
  // Fwd FFT
  fftw_plan fft_fwd_plan_;
  std::vector<double> fft_double_in_;
  fftw_complex fft_complex_out_[kFFTSize / 2 + 1];

  // Inverse FFT
  fftw_plan fft_rvr_plan_;
  std::vector<double> fft_double_out_;
};
