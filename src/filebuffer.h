#pragma once

#include <stdbool.h>

#include <array>
#include <atomic>
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
  explicit FileBuffer(const std::string& filename) : filename_{filename} {
    ImportFile(filename_);
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

  bool scramble_mode_{false};
  bool scramble_pending_{false};

  bool stutter_mode_{false};
  bool stutter_pending_{false};

  std::string filename_{};
  int num_channels_{2};

  LoopMode loop_mode_{LoopMode::loop_mode};
  double loop_len_{-1};

  int size_of_sixteenth_{0};
  std::atomic<int> audio_buffer_read_idx_{
      0};  // Atomic - read by audio thread, written by interpreter

  std::array<int, 16> scrambled_pattern_{0};

  int cur_sixteenth_{0};

  double incr_speed_{1};
  double cur_midi_idx_{0};

  double plooplen_{16};
  double poffset_{0};
  int pinc_{1};

  std::atomic<double> pitch_ratio_{1};  // Atomic - playback rate multiplier

 private:
  std::vector<double> audio_buffer_{};
};

}  // namespace SBAudio
