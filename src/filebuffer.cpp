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

void FileBuffer::SetPidx(int val) {
  poffset_ = abs(val - cur_sixteenth_) % 16;
}

void FileBuffer::SetPitch(double pitch_ratio) {
  // Simple: just update the playback rate
  // Audio thread will read from original buffer at variable rate
  pitch_ratio_.store(pitch_ratio);
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
void FileBuffer::SetPinc(int p) {
  pinc_ = p;
}

void FileBuffer::SetLoopLen(double bars) {
  if (bars != 0) {
    loop_len_ = bars;
    size_of_sixteenth_ = audio_buffer_.size() / loop_len_ / 16.;
  }
}

void FileBuffer::SetAudioBufferReadIdx(size_t pos) {
  if (pos < 0 || pos >= audio_buffer_.size()) {
    return;
  }
  audio_buffer_read_idx_ = pos;
}

std::vector<double>* FileBuffer::GetAudioBuffer() {
  return &audio_buffer_;
}

}  // namespace SBAudio
