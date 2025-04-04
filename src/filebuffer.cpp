#include "filebuffer.h"

#include <iostream>

#include "utils.h"

namespace SBAudio {

const std::array<std::string, 3> kLoopModeNames = {"LOOP", "STATIC", "SMUDGE"};

void FileBuffer::ImportFile(std::string filename) {
  AudioBufferDetails deetz = ImportFileContents(audio_buffer_, filename);
  num_channels = deetz.num_channels;
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
  else if (param == "play_for")
    play_for = value;
  else if (param == "next_action") {
    switch (static_cast<int>(value)) {
      case 1:
        next_action = PlayFirst;
        break;
      case 2:
        next_action = PlayNext;
        break;
      case 3:
        next_action = PlayPrevious;
        break;
      case 4:
        next_action = PlayRandom;
        break;
      case 5:
        next_action = Stop;
        break;
      case 0:
      default:
        next_action = NoAction;
    }
  }
}

void FileBuffer::SetPidx(int val) { poffset = abs(val - cur_sixteenth) % 16; }

void FileBuffer::SetPitch(double pitch_ratio) {
  pitch_ratio_ = pitch_ratio;
  pitched_audio_buffer_ =
      audioutils::resample(audio_buffer_, num_channels, pitch_ratio_);
  SetLoopLen(loop_len);
}

void FileBuffer::SetPOffset(int poffset) {
  if (poffset >= 0 && poffset <= 15) {
    poffset = poffset;
  }
}

void FileBuffer::SetPlooplen(int p) {
  if (p > 0 && p <= 16) {
    plooplen = p;
  }
}
void FileBuffer::SetPinc(int p) { pinc = p; }
void FileBuffer::SetLoopLen(double bars) {
  if (bars != 0) {
    loop_len = bars;
    size_of_sixteenth = GetAudioBuffer()->size() / bars / 16.;
  }
}

void FileBuffer::SetAudioBufferReadIdx(size_t pos) {
  if (pos < 0 || pos >= GetAudioBuffer()->size()) {
    return;
  }
  audio_buffer_read_idx = pos;
}

std::vector<double>* FileBuffer::GetAudioBuffer() {
  if (pitch_ratio_ != 1) {
    return &pitched_audio_buffer_;
  }
  return &audio_buffer_;
}

}  // namespace SBAudio
