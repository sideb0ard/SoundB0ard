#include "filebuffer.h"

#include <iostream>

#include "utils.h"

namespace SBAudio {

const std::array<std::string, 3> kLoopModeNames = {"LOOP", "STATIC", "SMUDGE"};

void FileBuffer::ImportFile(std::string filename) {
  AudioBufferDetails deetz = ImportFileContents(audio_buffer_, filename);
  num_channels_ = deetz.num_channels;
  SetLoopLen(1);
}

void FileBuffer::SetParam(std::string param, double value) {
  if (param == "idx") {
    if (value <= 100) {
      double pos = value / 100. * GetAudioBuffer()->size();
      SetAudioBufferReadIdx(pos);
    }
  } else if (param == "len")
    SetLoopLen(value);
  else if (param == "pidx")
    SetPidx(value);
  else if (param == "pitch")
    SetPitch(value);
  else if (param == "poffset")
    SetPOffset(value);
  else if (param == "plooplen")
    SetPlooplen(value);
  else if (param == "pinc")
    SetPinc(value);
}

void FileBuffer::SetPidx(int val) { poffset_ = abs(val - cur_sixteenth_) % 16; }

void FileBuffer::SetPitch(double pitch_ratio) {
  std::lock_guard<std::mutex> lock(pending_pitch_mutex_);
  if (pitch_ratio == 1) {
    pitch_ratio_ = pitch_ratio;
    return;
  }

  pending_pitch_ratio_ = pitch_ratio;
  pending_pitched_audio_buffer_ =
      std::async(std::launch::async, audioutils::resample, audio_buffer_,
                 num_channels_, pending_pitch_ratio_);
}

void FileBuffer::SetPOffset(int poffset) {
  if (poffset >= 0 && poffset <= 15) {
    poffset_ = poffset;
  }
}

void FileBuffer::SetPlooplen(int p) {
  if (p > 0 && p <= 16) {
    plooplen_ = p;
  }
}
void FileBuffer::SetPinc(int p) { pinc_ = p; }

void FileBuffer::SetLoopLen(double bars) {
  if (bars != 0) {
    loop_len_ = bars;
    size_of_sixteenth_ = GetAudioBuffer()->size() / loop_len_ / 16.;
  }
}

void FileBuffer::SetAudioBufferReadIdx(size_t pos) {
  if (pos < 0 || pos >= GetAudioBuffer()->size()) {
    return;
  }
  audio_buffer_read_idx_ = pos;
}

std::vector<double>* FileBuffer::GetAudioBuffer() {
  std::lock_guard<std::mutex> lock(pending_pitch_mutex_);
  // if (pending_pitch_mutex_.try_lock()) {
  if (pitch_ratio_ != 1) return &pitched_audio_buffer_;
  //}
  return &audio_buffer_;
}

void FileBuffer::CheckPendingRepitch() {
  if (!pending_pitch_ratio_ || !pending_pitched_audio_buffer_.valid()) return;

  if (pending_pitched_audio_buffer_.wait_for(std::chrono::seconds(0)) ==
      std::future_status::ready) {
    std::lock_guard<std::mutex> lock(pending_pitch_mutex_);
    pitched_audio_buffer_ = pending_pitched_audio_buffer_.get();
    pitch_ratio_ = pending_pitch_ratio_;

    pending_pitch_ratio_ = 0;

    size_of_sixteenth_ = pitched_audio_buffer_.size() / loop_len_ / 16.;
    double index_percent =
        100.0 / audio_buffer_.size() * audio_buffer_read_idx_;
    audio_buffer_read_idx_ =
        pitched_audio_buffer_.size() / 100.0 * index_percent;
  }
  //}
}

}  // namespace SBAudio
