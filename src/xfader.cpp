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
  } else {
    right_channel_.insert(idx);
  }
}

void XFader::Clear(int idx) {
  left_channel_.erase(idx);
  right_channel_.erase(idx);
}

void XFader::Update() {
  if (active_fade_) {
    double ramp_val = ramper_.Generate();
    if (fading_direction_ == LEFT) {
      xfader_position_ = -1.0 * ramp_val;
    } else {
      xfader_position_ = ramp_val;
    }
    if (ramp_val == 1) {
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

void XFader::SetXFaderPosition(double pos) {
  if (pos >= -1 && pos <= 1) {
    xfader_position_ = pos;
  }
}

void XFader::Fade(unsigned int left_or_right, double time_in_frames) {
  ramper_.Reset(time_in_frames);
  fading_direction_ = left_or_right;
  active_fade_ = true;
}

std::string XFader::Status() {
  std::stringstream ss;
  constexpr int xfader_graphic_size = 15;

  ss << "xpos: " << xfader_position_ << " [ ";
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

  return ss.str();
}
