#include "stepper.h"

#include <iostream>

namespace SBAudio {

int Sequencer::GenNext() {
  if (count_to_ == 0 || count_to_ == 1) return 0;

  int return_val = idx_;

  int steps = count_by_;
  while (steps > 0) {
    if (direction_ == Direction::Forward)
      idx_++;
    else
      idx_--;
    steps--;

    if (idx_ >= count_to_) {
      if (behavior_ == BoundaryBehavior::CycleRound) {
        idx_ = start_at_;
      } else {
        direction_ = Direction::Back;
        idx_ -= 2;
      }
    }
    if (idx_ < start_at_) {
      if (behavior_ == BoundaryBehavior::CycleRound) {
        idx_ = count_to_;
      } else {
        direction_ = Direction::Forward;
        idx_ += 2;
      }
    }
  }
  return return_val;
}

void Sequencer::SetParam(std::string param, double value) {
  if (param == "count_by") {
    if (value <= count_to_) count_by_ = value;
  } else if (param == "count_to") {
    if (value > start_at_) {
      count_to_ = value;
      if (idx_ >= count_to_) idx_ = start_at_;
    }
  } else if (param == "start_at") {
    if (value <= count_to_) {
      start_at_ = value;
      if (idx_ < start_at_) idx_ = start_at_;
    }
  } else if (param == "reset") {
    if (value == 1) Reset();
  } else if (param == "bounce") {
    if (value == 0)
      behavior_ = BoundaryBehavior::CycleRound;
    else
      behavior_ = BoundaryBehavior::BounceBack;
  }
}

}  // namespace SBAudio
