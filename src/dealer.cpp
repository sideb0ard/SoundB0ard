#include "dealer.h"

#include <iostream>

namespace SBAudio {

double Dealer::GenNext() {
  if (idx_ >= (int)sequence_.size()) return 0;
  if (sequence_.size() == 1) return sequence_[0];

  double return_val = sequence_.at(idx_);
  if (direction_ == Direction::Forward) {
    idx_ += step_;
  } else {
    idx_ = idx_ - step_;
  }

  int attempts = 5;
  while (attempts > 0 && (idx_ < 0 || idx_ >= (int)sequence_.size())) {
    attempts--;
    if (idx_ >= (int)sequence_.size()) {
      int diff = idx_ - (sequence_.size() - 1);
      if (behavior_ == BoundaryBehavior::CycleRound) {
        idx_ = diff;
      } else {
        direction_ = Direction::Back;
        idx_ = sequence_.size() - 1 - diff;
      }
    }
    if (idx_ < 0) {
      int diff = idx_ * -1;
      if (behavior_ == BoundaryBehavior::CycleRound) {
        idx_ = sequence_.size() - diff;
      } else {
        direction_ = Direction::Forward;
        idx_ = diff;
      }
    }
  }

  if (attempts == 0) {
    std::cerr << "RAN OUT OF ATTEMPTS!\n";
  }

  return return_val;
}

void Dealer::SetParam(std::string param, double value) {
  if (param == "step") {
    if (value <= sequence_.size()) step_ = value;
  } else if (param == "bounce") {
    if (value == 0)
      behavior_ = BoundaryBehavior::CycleRound;
    else
      behavior_ = BoundaryBehavior::BounceBack;
  }
}

}  // namespace SBAudio
