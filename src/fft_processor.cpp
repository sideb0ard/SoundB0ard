#include "fft_processor.h"

#include <iostream>

#include "defjams.h"

namespace {
// Helper function to wrap the phase between -pi and pi
double WrapPhase(double phase_in) {
  if (phase_in >= 0)
    return fmodf(phase_in + M_PI, 2.0 * M_PI) - M_PI;
  else
    return fmodf(phase_in - M_PI, -2.0 * M_PI) + M_PI;
}

}  // namespace

FFTProcessor::FFTProcessor() {
  std::cout << "FFFFFFT! proc yo!\n";

  fft_fwd_plan_ = fftw_plan_dft_r2c_1d(kFFTSize, fft_double_in_.data(),
                                       fft_complex_out_, FFTW_ESTIMATE);
  fft_rvr_plan_ = fftw_plan_dft_c2r_1d(kFFTSize, fft_complex_out_,
                                       fft_double_out_.data(), FFTW_ESTIMATE);
  for (int i = 0; i < kFFTSize / 2; i++) {
    center_frequencies_[i] = TWO_PI * i / kFFTSize;
    hann_window_buffer_[i] = 0.5 * (1 - cos((TWO_PI * i) / (kFFTSize - 1)));
  }
}

FFTProcessor::~FFTProcessor() {
  fftw_destroy_plan(fft_fwd_plan_);
  fftw_destroy_plan(fft_rvr_plan_);
  fftw_cleanup();
}

void FFTProcessor::ProcessFFT() {
  max_bin_index_ = 0;
  max_bin_value_ = 0;

  for (int i = 0; i < kFFTSize; i++) {
    fft_double_in_[i] =
        buffer_in_[(buffer_in_write_idx_ + i - kFFTSize + kBufferSize) %
                   kBufferSize] *
        hann_window_buffer_[i];
  }

  fftw_execute(fft_fwd_plan_);

  // the meat and potatoes!
  for (int i = 0; i < kFFTSize / 2; i++) {
    // std::cout << "DEM IMAGUINRR:" << fft_complex_out_[i][0] << " "
    //           << fft_complex_out_[i][1] << std::endl;
    double ampl = sqrt(fft_complex_out_[i][0] * fft_complex_out_[i][0] +
                       fft_complex_out_[i][1] * fft_complex_out_[i][1]);
    double phase = atan2(fft_complex_out_[i][1], fft_complex_out_[i][0]);

    // std::cout << "AMPL is:" << ampl << std::endl;
    // fft_complex_out_[i][0] = ampl;
    // fft_complex_out_[i][1] = 0;
    double phase_diff = phase - last_input_phases_[i];

    phase_diff = WrapPhase(phase_diff - center_frequencies_[i] * kHopSize);
    double bin_deviation = phase_diff * kFFTSize / kHopSize / TWO_PI;

    analysis_frequencies_[i] = i + bin_deviation;

    analysis_magnitudes_[i] = ampl;
    last_input_phases_[i] = phase;

    if (ampl > max_bin_value_) {
      max_bin_value_ = ampl;
      max_bin_index_ = i;
    }
  }

  double freq = analysis_frequencies_[max_bin_index_] * 44100 / kFFTSize;
  std::cout << "FREQ IS :" << freq << std::endl;

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
