#include <fx/waveshaper.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

WaveShaper::WaveShaper() {
  Init();

  type_ = WAVESHAPER;
  enabled_ = true;
}

void WaveShaper::Status(char *status_string) {
  snprintf(status_string, MAX_STATIC_STRING_SZ,
           "k_pos:%.2f k_neg:%.2f"
           "stages:%d invert:%d",
           m_arc_tan_k_pos, m_arc_tan_k_neg, m_stages, m_invert_stages);
}

stereo_val WaveShaper::Process(stereo_val input) {
  stereo_val out = {};
  out.left = ProcessAudio(input.left);
  out.right = ProcessAudio(input.right);

  return out;
}

void WaveShaper::SetParam(std::string name, double val) {
  if (name == "k_pos")
    SetArcTanKPos(val);
  else if (name == "k_neg")
    SetArcTanKNeg(val);
  else if (name == "stages")
    SetStages(val);
  else if (name == "invert")
    SetInvertStages(val);
}

double WaveShaper::GetParam(std::string name) { return 0; }

void WaveShaper::Init() {
  m_arc_tan_k_pos = 1;
  m_arc_tan_k_neg = 1;
  m_stages = 1;
  m_invert_stages = 0;  // off
}
void WaveShaper::SetArcTanKPos(double val) {
  if (val >= 0.1 && val <= 20)
    m_arc_tan_k_pos = val;
  else
    printf("Val must be between 0.1 and 20\n");
}

void WaveShaper::SetArcTanKNeg(double val) {
  if (val >= 0.1 && val <= 20)
    m_arc_tan_k_neg = val;
  else
    printf("Val must be between 0.1 and 20\n");
}

void WaveShaper::SetStages(unsigned int val) {
  if (val > 0 && val < 11)
    m_stages = val;
  else
    printf("Val must be between 1 and 10\n");
}

void WaveShaper::SetInvertStages(unsigned int val) {
  if (val < 2)
    m_invert_stages = val;
  else
    printf("Val must be 0 or 1\n");
}

double WaveShaper::ProcessAudio(double input) {
  double xn = input;
  for (unsigned int i = 0; i < m_stages; i++) {
    if (xn >= 0)
      xn = (1.0 / atan(m_arc_tan_k_pos)) * atan(m_arc_tan_k_pos * xn);
    else
      xn = (1.0 / atan(m_arc_tan_k_neg)) * atan(m_arc_tan_k_neg * xn);
    if (m_invert_stages && i % 2 == 0) xn *= -1.0;
  }
  return xn;
}
