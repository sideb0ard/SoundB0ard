#include "fft_processor.h"

#include <array>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "defjams.h"

namespace {
// Helper function to wrap the phase between -pi and pi
double WrapPhase(double phase_in) {
  if (phase_in >= 0)
    return fmodf(phase_in + M_PI, 2.0 * M_PI) - M_PI;
  else
    return fmodf(phase_in - M_PI, -2.0 * M_PI) + M_PI;
}

// Thanks to PitchDetect:
// https://github.com/cwilso/PitchDetect/blob/master/js/pitchdetect.js
std::array<std::string, 12> kNoteStrings{"C",  "C#", "D",  "D#", "E",  "F",
                                         "F#", "G",  "G#", "A",  "A#", "B"};

int NoteFromPitch(double frequency) {
  double note_num = 12. * (std::log(frequency / 440.) / std::log(2.));
  return std::round(note_num) + 69;
}
using Scale = std::array<int, 7>;
std::unordered_map<std::string, Scale> scales{
    {"C", {0, 2, 4, 5, 7, 9, 11}},   {"C#", {1, 3, 5, 6, 8, 10, 0}},
    {"D", {2, 4, 6, 7, 9, 11, 2}},   {"D#", {3, 5, 7, 8, 10, 0, 2}},
    {"E", {4, 6, 8, 9, 11, 1, 3}},   {"F", {5, 7, 9, 10, 0, 2, 4}},
    {"F#", {6, 8, 10, 11, 1, 3, 5}}, {"G", {7, 9, 11, 0, 2, 4, 6}},
    {"G#", {8, 10, 0, 1, 3, 5, 7}},  {"A", {9, 11, 1, 2, 4, 6, 8}},
    {"A#", {10, 0, 2, 3, 5, 7, 9}},  {"B", {11, 1, 3, 4, 6, 8, 10}},
};

std::string GuestimateKey(std::vector<int> notes) {
  std::stringstream ss;
  bool first_key_written = false;

  for (const auto& [key, value] : scales) {
    bool found = true;
    for (const auto& n : notes) {
      if (std::find(value.begin(), value.end(), n % 12) == value.end()) {
        found = false;
        break;
      }
    }

    if (found) {
      if (first_key_written) {
        ss << " ";
      } else {
        first_key_written = true;
      }
      ss << key;
    }
  }
  return ss.str();
}
}  // namespace

FFTProcessor::FFTProcessor() {
  std::cout << "FFFFFFT! proc yo!\n";

  fft_fwd_plan_ = fftw_plan_dft_r2c_1d(kFFTSize, fft_double_in_.data(),
                                       fft_complex_out_, FFTW_ESTIMATE);
  fft_rvr_plan_ = fftw_plan_dft_c2r_1d(kFFTSize, fft_complex_out_,
                                       fft_double_out_.data(), FFTW_ESTIMATE);
  for (int i = 0; i < kFFTSize / 2; i++) {
    center_frequencies_[i] = TWO_PI * (double)i / (double)kFFTSize;
  }
  for (int i = 0; i < kFFTSize; i++) {
    hann_window_buffer_[i] = 0.5 * (1 - cos(2 * M_PI * i / kFFTSize));
  }
}

void FFTProcessor::SetPitchSemitones(double pitch_diff) {
  if (pitch_diff >= -12 && pitch_diff <= 12) {
    pitch_diff_semis_ = pitch_diff;
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

  for (int i = 0; i < kFFTSize / 2; i++) {
    double ampl = sqrt(fft_complex_out_[i][0] * fft_complex_out_[i][0] +
                       fft_complex_out_[i][1] * fft_complex_out_[i][1]);
    double phase = atan2(fft_complex_out_[i][1], fft_complex_out_[i][0]);

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
  estimated_pitch_ = analysis_frequencies_[max_bin_index_] * 44100 / kFFTSize;
  notes_seen_[notes_seen_idx_++] = estimated_pitch_;
  if (notes_seen_idx_ == (int)notes_seen_.size()) notes_seen_idx_ = 0;

  for (int i = 0; i < kFFTSize / 2; i++) {
    synthesis_magnitudes_[i] = synthesis_frequencies_[i] = 0;
  }

  double pitch_shift = powf(2.0, pitch_diff_semis_ / 12.0);

  for (int i = 0; i < kFFTSize / 2; i++) {
    int new_bin = floor(i * pitch_shift + 0.5);

    if (new_bin <= kFFTSize / 2) {
      synthesis_magnitudes_[new_bin] += analysis_magnitudes_[i];
      synthesis_frequencies_[new_bin] = analysis_frequencies_[i] * pitch_shift;
    }
  }

  for (int i = 0; i < kFFTSize / 2; i++) {
    double ampl = synthesis_magnitudes_[i];
    double bin_deviation = synthesis_frequencies_[i] - i;

    double phase_diff = bin_deviation * TWO_PI * kHopSize / kFFTSize;

    double bin_center_freq = TWO_PI * i / kFFTSize;
    phase_diff += bin_center_freq * kHopSize;

    double out_phase = WrapPhase(last_output_phases_[i] + phase_diff);

    fft_complex_out_[i][0] = ampl * cos(out_phase);
    fft_complex_out_[i][1] = ampl * sin(out_phase);

    last_output_phases_[i] = out_phase;
  }

  fftw_execute(fft_rvr_plan_);

  double scale_val = 0.004;
  for (int i = 0; i < kFFTSize; i++) {
    int out_idx = (buffer_out_write_idx_ + i) % kBufferSize;
    buffer_out_[out_idx] +=
        fft_double_out_[i] * hann_window_buffer_[i] * scale_val;
    // fft_double_out_[i] / kFFTSize;  // * hann_window_buffer_[i];
    // std::cout << "BUFOOT:" << buffer_out_[out_idx] << std::endl;
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

std::vector<int> FFTProcessor::TallyNotesSeen() {
  // std::vector<int> recent_notes(notes_seen_.be
  std::make_heap(notes_seen_.begin(), notes_seen_.end());
  return std::vector<int>(notes_seen_.begin(), notes_seen_.begin() + 3);
}

std::string FFTProcessor::GetPitch() {
  return kNoteStrings[NoteFromPitch(estimated_pitch_) % 12];
}

std::string FFTProcessor::GetKey() { return GuestimateKey(TallyNotesSeen()); }
