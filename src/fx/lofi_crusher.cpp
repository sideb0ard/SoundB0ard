#include <fx/lofi_crusher.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

#include <sstream>

LoFiCrusher::LoFiCrusher() {
  Init();
  type_ = fx_type::LOFI_CRUSHER;
  enabled_ = true;
}

std::string LoFiCrusher::Status() {
  std::stringstream ss;
  ss << "LoFiCrusher - ";
  ss << "bitdepth:" << bitdepth_;
  ss << " sample_hold:" << sample_hold_freq_;
  ss << " destruct:" << destruct_;
  return ss.str();
}

StereoVal LoFiCrusher::Process(StereoVal input) {
  // Sample rate reduction via sample-and-hold
  phasor_ += sample_hold_freq_;
  if (phasor_ >= 1.0) {
    phasor_ -= 1.0;

    // Bit depth reduction with optional destructive quantization
    if (destruct_ > 0.0) {
      // Use destructive quantization (from Decimate)
      last_left_ = DestructQuantize(input.left, destruct_);
      last_right_ = DestructQuantize(input.right, destruct_);
    } else {
      // Clean quantization (from BitCrush)
      last_left_ = Quantize(input.left, step_);
      last_right_ = Quantize(input.right, step_);
    }
  }

  StereoVal output;
  output.left = last_left_;
  output.right = last_right_;
  return output;
}

void LoFiCrusher::SetParam(std::string name, double val) {
  if (name == "bitdepth") {
    SetBitdepth(static_cast<int>(val));
  } else if (name == "sample_hold_freq") {
    SetSampleHoldFreq(val);
  } else if (name == "destruct") {
    SetDestruct(static_cast<float>(val));
  }
  Update();
}

void LoFiCrusher::Init() {
  bitdepth_ = 8;
  sample_hold_freq_ = 1.0;
  destruct_ = 0.0;
  phasor_ = 0.0;
  last_left_ = 0.0;
  last_right_ = 0.0;
  samples_left_ = 0;
  Update();
}

void LoFiCrusher::Update() {
  // Calculate quantization step size
  step_ = 2.0f * powf(0.5f, static_cast<float>(bitdepth_));
  inv_step_ = 1.0f / step_;
}

void LoFiCrusher::SetBitdepth(int val) {
  if (val >= 1 && val <= 16) {
    bitdepth_ = val;
    Update();
  } else {
    printf("bitdepth must be between 1 and 16 (got %d)\n", val);
  }
}

void LoFiCrusher::SetSampleHoldFreq(double val) {
  sample_hold_freq_ = clamp(0.0, 1.0, val);
}

void LoFiCrusher::SetDestruct(float val) {
  if (val >= 0.0f && val <= 1.0f) {
    destruct_ = val;
  } else {
    printf("destruct must be between 0 and 1 (got %f)\n", val);
  }
}

float LoFiCrusher::Quantize(float sample, float step) {
  // Clean quantization
  return step * floorf((sample * inv_step_) + 0.5f);
}

float LoFiCrusher::DestructQuantize(float sample, float destruct) {
  // Destructive quantization (from Decimate's "destroy" function)
  // Favor low end frequencies - more exciting there
  sample = sample * sample * sample;

  long scale = static_cast<long>(512 * (1.0f - destruct));
  if (scale == 0) scale = 2;
  long X = static_cast<long>(scale * sample);

  float quantized = static_cast<float>(X) / static_cast<float>(scale);

  // Add feedback/accumulation for extra destruction
  if (destruct > 0.5f) {
    quantized += sample * (destruct - 0.5f) * 0.2f;
  }

  return clamp(-1.0f, 1.0f, quantized);
}
