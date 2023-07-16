#include <iostream>

#include "stepper.h"

namespace SBAudio {

double Stepper::GenNext() {
  if (sequence_.size() == 0) return 0;
  if (sequence_.size() == 1) return sequence_[0];

  double return_val = sequence_.at(idx_);
  std::cout << "RET VAL IS:" << return_val << " from IDX:" << idx_ << std::endl;

  int steps = step_;
  while (steps > 0) {
    if (direction_ == Direction::Forward)
      idx_++;
    else
      idx_--;
    steps--;

    std::cout << "IDX is:" << idx_ << " STEPS IS:" << steps << std::endl;

    if (idx_ >= (int)sequence_.size()) {
      std::cout << "TOO BIG!\n";
      if (behavior_ == BoundaryBehavior::CycleRound) {
        idx_ = start_at_;
        std::cout << "CYCLE - now IDX is " << idx_ << std::endl;
      } else {
        direction_ = Direction::Back;
        idx_ -= 2;
        std::cout << "BOUNC - now IDX is " << idx_ << std::endl;
      }
    }
    if (idx_ < 0) {
      std::cout << "TOO WEE!\n";
      if (behavior_ == BoundaryBehavior::CycleRound) {
        idx_ = count_to_;
        std::cout << "CYCLE - now IDX is " << idx_ << std::endl;
      } else {
        direction_ = Direction::Forward;
        idx_ += 2;
        std::cout << "BOUNC - now IDX is " << idx_ << std::endl;
      }
    }
  }

  std::cout << "IDX NOW now:" << idx_ << "\n" << std::endl;

  return return_val;
}

void Stepper::SetParam(std::string param, double value) {
  if (param == "step") {
    if (value <= sequence_.size()) step_ = value;
  } else if (param == "count_to") {
    if (value <= sequence_.size()) count_to_ = value;
  } else if (param == "start_at") {
    if (value <= sequence_.size()) start_at_ = value;
  } else if (param == "bounce") {
    if (value == 0)
      behavior_ = BoundaryBehavior::CycleRound;
    else
      behavior_ = BoundaryBehavior::BounceBack;
  }
}

}  // namespace SBAudio
