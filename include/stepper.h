#pragma once

#include <vector>

#include "defjams.h"

namespace SBAudio {

enum Direction {
  Forward,
  Back,
};

enum class BoundaryBehavior {
  CycleRound,
  BounceBack,
};

class Sequencer {
 public:
  Sequencer(int len) : count_to_{len} {}
  ~Sequencer() = default;

  void SetParam(std::string param, double value);
  int GenNext();
  void Reset() {
    count_by_ = 1;
    start_at_ = 0;
    idx_ = 0;
  }

 public:
  BoundaryBehavior behavior_{BoundaryBehavior::BounceBack};
  int count_by_{1};
  int start_at_{0};
  int count_to_{0};

  int idx_{0};
  Direction direction_{Direction::Forward};
};

class Stepper {
 public:
  Stepper() = default;
  Stepper(std::vector<double> sequence)
      : sequence_{sequence}, sequencer_{(int)sequence.size()} {}
  ~Stepper() = default;

  void SetSequence(std::vector<double> sequence) {
    sequence_ = sequence;
    sequencer_.Reset();
    sequencer_.count_to_ = sequence.size();
  }
  void SetParam(std::string param, double value) {
    sequencer_.SetParam(param, value);
  }
  double GenNext() {
    int idx = sequencer_.GenNext();
    if (idx < static_cast<int>(sequence_.size())) {
      return sequence_[idx];
    }
    return 0;
  }
  void Reset() {
    sequencer_.Reset();
    sequencer_.count_to_ = sequence_.size();
  }

 public:
  std::vector<double> sequence_;
  Sequencer sequencer_{0};
};

}  // namespace SBAudio
