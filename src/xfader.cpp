#include "xfader.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "utils.h"

void XFader::Assign(int idx, unsigned int left_or_right) {
  bool is_in_left = left_channel_.contains(idx);
  bool is_in_right = right_channel_.contains(idx);

  if ((is_in_left && left_or_right == RIGHT) ||
      (is_in_right && left_or_right == LEFT)) {
    std::cout << "Woof! " << idx << " is already assigned to other channel!"
              << std::endl;
    return;
  }

  if (left_or_right == LEFT) {
    left_channel_.insert(idx);
  } else if (left_or_right == RIGHT) {
    right_channel_.insert(idx);
  }
}

void XFader::Clear() {
  left_channel_.clear();
  right_channel_.clear();
}

void XFader::Update(mixer_timing_info tinfo) {
  frames_per_midi_tick_ = tinfo.frames_per_midi_tick;
  if (active_fade_) {
    if (fading_direction_ == LEFT) {
      xfader_position_ -= increment_;
      if (xfader_position_ <= xfader_destination_position_)
        active_fade_ = false;
    } else {
      xfader_position_ += increment_;
      if (xfader_position_ >= xfader_destination_position_)
        active_fade_ = false;
    }
  }
}

double XFader::GetValueFor(int idx) {
  bool is_in_left = left_channel_.find(idx) != left_channel_.end();
  bool is_in_right = right_channel_.find(idx) != right_channel_.end();
  if (is_in_left) {
    if (xfader_position_ <= 0) {
      return 1;
    } else {
      return 1 - std::abs(xfader_position_);
    }
  }
  if (is_in_right) {
    if (xfader_position_ >= 0) {
      return 1;
    } else {
      return 1 - std::abs(xfader_position_);
    }
  }
  return 1;
}

void XFader::Set(std::string var, double val) {
  if (var == "xpos") {
    SetXFaderPosition(val);
  } else if (var == "fade_tics") {
    SetFadeTimeMidiTicks(val);
  }
}

void XFader::SetXFaderPosition(double pos) {
  if (pos >= -1 && pos <= 1 && pos != xfader_position_) {
    xfader_destination_position_ = pos;
    double distance = std::abs(xfader_position_ - xfader_destination_position_);
    increment_ = distance / (xfade_time_midi_tics_ * frames_per_midi_tick_);
    active_fade_ = true;
    if (xfader_destination_position_ < xfader_position_)
      fading_direction_ = LEFT;
    else
      fading_direction_ = RIGHT;
  }
}

void XFader::SetFadeTimeMidiTicks(double tics) {
  xfade_time_midi_tics_ = tics;
}

std::string XFader::Status() {
  std::stringstream ss;
  constexpr int xfader_graphic_size = 15;

  ss << "[ ";
  for (auto it = left_channel_.begin(); it != left_channel_.end(); ++it) {
    ss << *it << " ";
  }
  ss << "]";

  ss << " <";
  int xfade_graphic_pos =
      scale(xfader_position_, -1, 1, 0, xfader_graphic_size);
  int num_left_pos = xfade_graphic_pos - 1;
  if (num_left_pos > 0) {
    for (int i = 0; i < num_left_pos; i++) {
      ss << "-";
    }
  }
  ss << "I";

  int num_right_pos = xfader_graphic_size - xfade_graphic_pos - 1;
  if (num_right_pos > 0) {
    for (int i = 0; i < num_right_pos; i++) {
      ss << "-";
    }
  }
  ss << "> ";

  ss << "[ ";
  for (auto it = right_channel_.begin(); it != right_channel_.end(); ++it) {
    ss << *it << " ";
  }
  ss << "]";
  ss << " xpos: " << xfader_position_ << " fade_tics:" << xfade_time_midi_tics_;

  return ss.str();
}
