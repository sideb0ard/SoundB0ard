#include <vector>

#include "defjams.h"

namespace SBAudio {

enum class Direction {
  Forward,
  Back,
};

enum class BoundaryBehavior {
  CycleRound,
  BounceBack,
};

class Dealer {
 public:
  Dealer() = default;
  Dealer(std::vector<double> sequence) : sequence_{sequence} {}
  ~Dealer() = default;

  void SetSequence(std::vector<double> sequence) { sequence_ = sequence; }
  void SetParam(std::string param, double value);
  double GenNext();

 public:
  std::vector<double> sequence_;
  Direction direction_{Direction::Forward};
  BoundaryBehavior behavior_{BoundaryBehavior::BounceBack};
  int step_{1};
  int idx_{0};
};

}  // namespace SBAudio
