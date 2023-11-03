#include "fft_processor.h"

#include <iostream>

FFTProcessor::FFTProcessor() {
  std::cout << "FFFFFFT! proc yo!\n";

  buffer_in_ = std::vector<double>(kBufferSize, 0);
  buffer_out_ = std::vector<double>(kBufferSize, 0);

  fft_double_in_ = std::vector<double>(kFFTSize, 0);
  fft_double_out_ = std::vector<double>(kFFTSize, 0);
  fft_fwd_plan_ = fftw_plan_dft_r2c_1d(kFFTSize, fft_double_in_.data(),
                                       fft_complex_out_, FFTW_ESTIMATE);
  fft_rvr_plan_ = fftw_plan_dft_c2r_1d(kFFTSize, fft_complex_out_,
                                       fft_double_out_.data(), FFTW_ESTIMATE);
}

FFTProcessor::~FFTProcessor() {
  fftw_destroy_plan(fft_fwd_plan_);
  fftw_destroy_plan(fft_rvr_plan_);
  fftw_cleanup();
}

void FFTProcessor::ProcessFFT() {
  for (int i = 0; i < kFFTSize; i++) {
    fft_double_in_[i] =
        buffer_in_[(buffer_in_write_idx_ + i - kFFTSize + kBufferSize) %
                   kBufferSize];
  }

  fftw_execute(fft_fwd_plan_);

  for (int i = 0; i < kFFTSize / 2; i++) {
    // std::cout << "DEM IMAGUINRR:" << fft_complex_out_[i][0] << " "
    //           << fft_complex_out_[i][1] << std::endl;
    double ampl = sqrt(fft_complex_out_[i][0] * fft_complex_out_[i][0] +
                       fft_complex_out_[i][1] * fft_complex_out_[i][1]);
    double phase = atan2(fft_complex_out_[i][1], fft_complex_out_[i][0]);
    // std::cout << "AMPL is:" << ampl << std::endl;
    fft_complex_out_[i][0] = ampl;
    fft_complex_out_[i][1] = 0;
  }

  fftw_execute(fft_rvr_plan_);

  for (int i = 0; i < kFFTSize; i++) {
    int out_idx = (buffer_out_write_idx_ + i) % kBufferSize;
    buffer_out_[out_idx] += fft_double_out_[i] / kFFTSize;
  }
}

double FFTProcessor::ProcessData(double val) {
  //////////////// FFFFFFT! ///////////////////////
  buffer_in_[buffer_in_write_idx_] = val;
  buffer_in_write_idx_++;
  if (buffer_in_write_idx_ >= kBufferSize) buffer_in_write_idx_ = 0;

  double fft_out = buffer_out_[buffer_out_read_idx_];
  buffer_out_[buffer_out_read_idx_] = 0;
  fft_out *= (double)kHopSize / (double)kFFTSize;

  buffer_out_read_idx_++;
  if (buffer_out_read_idx_ >= kBufferSize) {
    buffer_out_read_idx_ = 0;
  }

  fft_hop_count_++;
  if (fft_hop_count_ % kHopSize == 0) {
    ProcessFFT();
    buffer_out_write_idx_ = (buffer_out_write_idx_ + kHopSize) % kBufferSize;
  }
  return fft_out;
}
