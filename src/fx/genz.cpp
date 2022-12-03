#include <fx/genz.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <iostream>
#include <sstream>

GenZ::GenZ() {
  Reset();
  enabled_ = true;
}

std::string GenZ::Status() {
  std::stringstream ss;
  ss << "signal:" << signal_ << " rate:" << rate_;

  return ss.str();
}

StereoVal GenZ::Process(StereoVal input) {
  // do something
  signal_ = scale(counter_, 0, loop_len_in_frames_ * rate_, 0, 1);
  counter_++;

  return input;
}

void GenZ::SetParam(std::string name, double val) {
  if (name == "rate") {
    rate_ = val;
  }
}

void GenZ::Reset() {
  counter_ = 0;
  signal_ = 0;
}

void GenZ::EventNotify(broadcast_event event, mixer_timing_info tinfo) {
  if (event.type == TIME_MIDI_TICK) {
    loop_len_in_frames_ = tinfo.loop_len_in_frames;
    if (tinfo.midi_tick % (int)(rate_ * PPBAR) == 0) {
      Reset();
    }
  }
  if (tinfo.is_sixteenth) {
    std::cout << "SIG:" << signal_ << std::endl;
  }
}
