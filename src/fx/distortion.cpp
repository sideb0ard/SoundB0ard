#include <fx/distortion.h>
#include <math.h>
#include <mixer.h>
#include <stdlib.h>

#include <sstream>

Distortion::Distortion() {
  type_ = DISTORTION;
  enabled_ = true;
  // m_threshold_ = 0.707;
  m_threshold_ = 0.507;
}

std::string Distortion::Status() {
  std::stringstream ss;
  ss << "Distortion! threshold:" << m_threshold_;
  return ss.str();
}

StereoVal Distortion::Process(StereoVal input) {
  StereoVal out = {};

  if (input.left >= 0)
    out.left = fmin(input.left, m_threshold_);
  else
    out.left = fmax(input.left, -m_threshold_);
  out.left /= m_threshold_;

  if (input.right >= 0)
    out.right = fmin(input.right, m_threshold_);
  else
    out.right = fmax(input.right, -m_threshold_);
  out.right /= m_threshold_;

  return out;
}

void Distortion::SetParam(std::string name, double val) {
  if (name == "threshold") SetThreshold(val);
}

void Distortion::SetThreshold(double val) {
  if (val >= 0.01 && val <= 1.0)
    m_threshold_ = val;
  else
    printf("Val must be between 0.01 and 1\n");
}
