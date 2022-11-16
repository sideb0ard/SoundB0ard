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
  ss << "counter_:" << counter_;

  return ss.str();
}

stereo_val GenZ::Process(stereo_val input) {
  // do something
  signal_ = scale(counter_, 0, loop_len_in_frames_, 0, 1);
  counter_++;

  return input;
}

void GenZ::SetParam(std::string name, double val) {
  if (name == "some_setting") {
    // update it;
    // Update();
  }
}

void GenZ::Reset() {
  counter_ = 0;
  signal_ = 0;
}

void GenZ::EventNotify(broadcast_event event, mixer_timing_info tinfo) {
  if (event.type == TIME_MIDI_TICK) {
    loop_len_in_frames_ = tinfo.loop_len_in_frames;
    if (tinfo.is_start_of_loop) {
      Reset();
    }
  }
  if (tinfo.is_sixteenth) {
    std::cout << "RAMP:" << signal_ << std::endl;
  }
}
