#include <fx/bitcrush.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <sstream>

BitCrush::BitCrush() {
  Init();
  type_ = BITCRUSH;
  enabled_ = true;
}

std::string BitCrush::Status() {
  std::stringstream ss;
  ss << "bitdepth:" << bitdepth_;
  ss << " bitrate:" << bitrate_;
  ss << " sample_hold_freq:" << sample_hold_freq_;

  return ss.str();
}

StereoVal BitCrush::Process(StereoVal input) {
  phasor_ += sample_hold_freq_;
  if (phasor_ >= 1) {
    phasor_ -= 1;
    last_left_ = step_ * floor((input.left * inv_step_) + 0.5);
    last_right_ = step_ * floor((input.left * inv_step_) + 0.5);
  }

  input.left = last_left_;
  input.right = last_right_;
  return input;
}

void BitCrush::SetParam(std::string name, double val) {
  if (name == "bitdepth")
    SetBitdepth(val);
  else if (name == "bitrate")
    SetBitrate(val);
  else if (name == "sample_hold_freq")
    SetSampleHoldFreq(val);
  Update();
}

void BitCrush::Init() {
  bitdepth_ = 6;
  bitrate_ = 4096;
  sample_hold_freq_ = 1;
  phasor_ = 0;
  last_left_ = 0;
  last_right_ = 0;
  Update();
}

void BitCrush::Update() {
  step_ = 2 * fast_pow(0.5, bitdepth_);
  inv_step_ = 1 / step_;
}

void BitCrush::SetBitdepth(int val) {
  if (val >= 1 && val <= 16) {
    bitdepth_ = val;
    Update();
  } else
    printf("Val must be between 1 and 16:%d\n", val);
}

void BitCrush::SetSampleHoldFreq(double val) {
  val = clamp(0, 1, val);
  sample_hold_freq_ = val;
  Update();
}

void BitCrush::SetBitrate(int val) {
  if (val >= 200 && val <= SAMPLE_RATE) {
    bitrate_ = val;
    Update();
  } else
    printf("Val must be between 200 and %d\n", SAMPLE_RATE);
}
