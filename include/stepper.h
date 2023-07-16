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
      : sequence_{sequence}, count_to_{sequence.size()} {}
  ~Stepper() = default;

  void SetSequence(std::vector<double> sequence) {
    sequence_ = sequence;
    count_to_ = sequence.size();
  }
  void SetParam(std::string param, double value);
  double GenNext();

 public:
  std::vector<double> sequence_;
  BoundaryBehavior behavior_{BoundaryBehavior::BounceBack};
  int step_{1};
  int start_at_{0};
  size_t count_to_{0};

  int idx_{0};
  Direction direction_{Direction::Forward};
};

}  // namespace SBAudio
