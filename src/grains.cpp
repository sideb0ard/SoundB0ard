#include <audioutils.h>
#include <grains.h>

#include <iostream>

namespace {
void check_idx(int *index, int buffer_len) {
  while (*index < 0.0) *index += buffer_len;
  while (*index >= buffer_len) *index -= buffer_len;
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
  reverse_mode = params.reverse_mode;

  grain_len_frames = params.dur_frames;
  audiobuffer_cur_pos = params.starting_idx;
  if (reverse_mode) {
    audiobuffer_cur_pos =
        params.starting_idx + (grain_len_frames * audiobuffer_num_channels) - 1;
    incr = -1.0;
  }

  active = true;
}

void SoundGrainSample::SetReadIdx(int idx) {
  audiobuffer_cur_pos = idx;
}

StereoVal SoundGrainSample::Generate() {
  StereoVal out = {0., 0.};
  if (!active) return out;

  if (degrade_by > 0) {
    if (rand() % 100 < degrade_by) return out;
  }

  int read_idx = (int)audiobuffer_cur_pos;
  check_idx(&read_idx, audio_buffer->size());
  out.left = (*audio_buffer)[read_idx];
  if (audiobuffer_num_channels == 1) {
    out.right = out.left;
  } else if (audiobuffer_num_channels == 2) {
    int read_idx_right = read_idx + 1;
    check_idx(&read_idx_right, audio_buffer->size());
    out.right = (*audio_buffer)[read_idx_right];
  }

  audiobuffer_cur_pos += (incr * audiobuffer_num_channels);
  grain_frame_counter++;

  if (grain_frame_counter > grain_len_frames) {
    active = false;
  }

  return out;
}

}  // namespace SBAudio
