#include <audioutils.h>
#include <grains.h>

#include <iostream>

namespace {
void check_idx(int *index, int buffer_len) {
  while (*index < 0.0) *index += buffer_len;
  while (*index >= buffer_len) *index -= buffer_len;
}

constexpr int M = 32;

double resample(double orig_sample_rate, double pitched_sample_rate,
                std::vector<double> *audio_buffer, double buffer_offset,
                int num_channels, double position_j) {
  double J = position_j * orig_sample_rate / pitched_sample_rate;
  int read_idx = (int)(buffer_offset + J);
  check_idx(&read_idx, audio_buffer->size());

  int k_lo = J - M / 2 * num_channels;
  int k_hi = J + M / 2 * num_channels;

  double freq_scaler =
      std::min(orig_sample_rate, pitched_sample_rate) / orig_sample_rate;

  double sinc_summation = 0;
  for (int k = k_lo; k < k_hi; k += num_channels) {
    int k_idx = read_idx + k;
    check_idx(&k_idx, audio_buffer->size());
    double k_val = (*audio_buffer)[k_idx];

    int k_minus_j_idx = (int)(k_idx - J * num_channels);
    check_idx(&k_minus_j_idx, audio_buffer->size());
    double k_minus_j_val = (*audio_buffer)[k_minus_j_idx];

    sinc_summation += audioutils::sinc(freq_scaler * k_minus_j_val) *
                      audioutils::blackman(k - J + M / 2, M) * k_val;
  }
  double bj =
      std::min(1.0, orig_sample_rate / pitched_sample_rate) * sinc_summation;

  return bj;
}

}  // namespace

namespace SBAudio {

void SoundGrainSample::Initialize(SoundGrainParams params) {
  if (params.grain_type != type) {
    std::cerr << "Putting DIESEL IN AN UNLEADED!!\n";
    return;
  }

  grain_frame_counter = 0;
  incr = 1;

  audio_buffer = params.audio_buffer;
  audiobuffer_num_channels = params.num_channels;
  degrade_by = params.degrade_by;
  pitch_scale = params.pitch_scale;
  reverse_mode = params.reverse_mode;

  if (pitch_scale != 1) {
    grain_len_frames = params.dur_frames * 1 / pitch_scale;
    pitched_audio_buffer =
        std::vector<double>(grain_len_frames * audiobuffer_num_channels, 0);
    double pitched_sample_rate = SAMPLE_RATE * 1 / pitch_scale;
    for (size_t i = 0; i < pitched_audio_buffer.size(); i++) {
      pitched_audio_buffer[i] =
          resample(SAMPLE_RATE, pitched_sample_rate, audio_buffer,
                   params.starting_idx, params.num_channels, i);
    }
    audiobuffer_cur_pos = 0;
    if (reverse_mode) {
      audiobuffer_cur_pos = pitched_audio_buffer.size() - 2;
      incr = -1.0;
    }
  } else {
    grain_len_frames = params.dur_frames;
    audiobuffer_cur_pos = params.starting_idx;
    if (reverse_mode) {
      audiobuffer_cur_pos = params.starting_idx +
                            (grain_len_frames * audiobuffer_num_channels) - 1;
      incr = -1.0;
    }
  }

  // 1. RA - SAMPLE_RATE
  // 2. A - audio_buffer
  // 3. FA - frequency 1
  // 4. FB - frequency 2
  // 5. j - 0 .. length of audio_buffer * FB
  // 6. RB <- RA X FA / FB
  // 7. M <- audio_buffer->size()
  // 8. w(m, M) <- window // blackman
  // 9. e(A, k) - extraction - single shot or looped
  // 10. bj <- windowed sinc with RA, RB, j, M, m(..) and e(..)
  // 11. return bj

  active = true;
}

void SoundGrainSample::SetReadIdx(int idx) { audiobuffer_cur_pos = idx; }

StereoVal SoundGrainSample::Generate() {
  StereoVal out = {0., 0.};
  if (!active) return out;

  if (degrade_by > 0) {
    if (rand() % 100 < degrade_by) return out;
  }

  std::vector<double> *buffer = audio_buffer;
  if (pitch_scale != 1) {
    buffer = &pitched_audio_buffer;
  }

  int read_idx = (int)audiobuffer_cur_pos;
  check_idx(&read_idx, buffer->size());
  out.left = (*buffer)[read_idx];
  if (audiobuffer_num_channels == 1) {
    out.right = out.left;
  } else if (audiobuffer_num_channels == 2) {
    int read_idx_right = read_idx + 1;
    check_idx(&read_idx_right, buffer->size());
    out.right = (*buffer)[read_idx_right];
  }

  audiobuffer_cur_pos += (incr * audiobuffer_num_channels);
  grain_frame_counter++;

  if (grain_frame_counter > grain_len_frames) {
    active = false;
  }

  return out;
}

}  // namespace SBAudio
