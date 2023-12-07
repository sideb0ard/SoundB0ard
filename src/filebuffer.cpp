#include "filebuffer.h"

#include <iostream>

#include "utils.h"

namespace SBAudio {

const std::array<std::string, 3> kLoopModeNames = {"LOOP", "STATIC", "SMUDGE"};

void FileBuffer::ImportFile(std::string filename) {
  std::cout << "FB IMPORT FILE YO!:" << filename << std::endl;
  AudioBufferDetails deetz = ImportFileContents(audio_buffer, filename);
  num_channels = deetz.num_channels;
  SetLoopLen(1);
}

void FileBuffer::SetLoopLen(double bars) {
  if (bars != 0) {
    loop_len = bars;
    size_of_sixteenth = audio_buffer.size() / bars / 16.;
  }
}

void FileBuffer::SetAudioBufferReadIdx(size_t pos) {
  if (pos < 0 || pos >= audio_buffer.size()) {
    return;
  }
  audio_buffer_read_idx = pos;
  std::cout << "YO SETTIN READ IDX TO : " << pos << std::endl;
}

}  // namespace SBAudio
