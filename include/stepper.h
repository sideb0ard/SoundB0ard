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

class Stepper {
 public:
  Stepper() = default;
  Stepper(std::vector<double> sequence)
      : sequence_{sequence}, count_to_{(int)sequence.size()} {}
  ~Stepper() = default;

  void SetSequence(std::vector<double> sequence) {
    sequence_ = sequence;
    count_to_ = sequence.size();
  }
  void SetParam(std::string param, double value);
  double GenNext();
  void Reset() {
    count_by_ = 1;
    start_at_ = 0;
    idx_ = 0;
    count_to_ = sequence_.size();
  }

 public:
  std::vector<double> sequence_;
  BoundaryBehavior behavior_{BoundaryBehavior::BounceBack};
  int count_by_{1};
  int start_at_{0};
  int count_to_{0};

  int idx_{0};
  Direction direction_{Direction::Forward};
};

}  // namespace SBAudio
