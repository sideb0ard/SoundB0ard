#pragma once

#include <stdbool.h>

#include <array>
#include <string>
#include <vector>

namespace SBAudio {

enum LoopMode {
  loop_mode,
  static_mode,
  smudge_mode,
};

enum NextAction {
  NoAction,
  PlayFirst,
  PlayNext,
  PlayPrevious,
  PlayRandom,
  Stop,
};

class FileBuffer {
 public:
  FileBuffer() = default;
  FileBuffer(std::string filename) : filename{filename} {
    ImportFile(filename);
  };
  ~FileBuffer() = default;

  void ImportFile(std::string filename);

  void SetParam(std::string param, double value);
  void SetLoopLen(double bars);
  void SetAudioBufferReadIdx(size_t position);
  void SetPidx(int val);
  void SetPOffset(int poffset);
  void SetPlooplen(int plooplen);
  void SetPinc(int pinc);
  void SetPitch(double pitch_ratio);
  void SetLoopMode(unsigned int m);
  void SetScramblePending();
  void SetStutterPending();

  std::vector<double>* GetAudioBuffer();

  bool scramble_mode{false};
  bool scramble_pending{false};

  bool stutter_mode{false};
  bool stutter_pending{false};

  std::string filename{};
  int num_channels{2};

  LoopMode loop_mode{LoopMode::loop_mode};
  int loop_len{-1};

  int size_of_sixteenth{0};
  int audio_buffer_read_idx{0};

  std::array<int, 16> scrambled_pattern{0};

  int cur_sixteenth{0};

  double incr_speed{1};
  double cur_midi_idx{0};

  double plooplen{16};
  double poffset{0};
  int pinc{1};
  bool pbounce{false};
  bool preverse{false};

  int play_for{1};  // loops
  NextAction next_action{PlayFirst};

  double pitch_ratio_{1};

 private:
  std::vector<double> audio_buffer_{};
  std::vector<double> pitched_audio_buffer_{};
};

}  // namespace SBAudio
