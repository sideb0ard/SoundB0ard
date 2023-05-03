#include <set>

#include "defjams.h"
#include "ramp.h"

class XFader {
 public:
  XFader() = default;
  ~XFader() = default;

  void Assign(int soundgenerator_idx, unsigned int left_or_right);
  void Clear(int soundgenerator_idx);

  void Update();
  double GetValueFor(int soundgenerator_idx);
  void Fade(unsigned int left_or_right, double time_in_frames);
  void SetXFaderPosition(double pos);  // between -1 and 1;
  std::string Status();

 private:
  std::set<int> left_channel_{};
  std::set<int> right_channel_{};

  double xfader_position_{0};  // from -1 to 1
  bool active_fade_{false};
  unsigned int fading_direction_{LEFT};

  Ramp ramper_;
};
