#include <fx/distortion.h>
#include <math.h>
#include <mixer.h>
#include <stdlib.h>

#include <sstream>

Distortion::Distortion() {
  Init();
  type_ = fx_type::DISTORTION;
  enabled_ = true;
}

void Distortion::Init() {
  mode_ = DistortionMode::HARD_CLIP;
  threshold_ = 0.5;
  drive_ = 1.0;
}

std::string Distortion::Status() {
  std::stringstream ss;
  ss << "Distortion - ";

  // Show mode name
  switch (mode_) {
    case DistortionMode::HARD_CLIP:
      ss << "HARD_CLIP";
      break;
    case DistortionMode::SOFT_CLIP:
      ss << "SOFT_CLIP";
      break;
    case DistortionMode::TUBE:
      ss << "TUBE";
      break;
    case DistortionMode::FOLDBACK:
      ss << "FOLDBACK";
      break;
  }

  ss << " threshold:" << threshold_;
  ss << " drive:" << drive_;
  return ss.str();
}

StereoVal Distortion::Process(StereoVal input) {
  StereoVal out = {};

  // Apply drive (input gain)
  double left_driven = input.left * drive_;
  double right_driven = input.right * drive_;

  // Process based on mode
  switch (mode_) {
    case DistortionMode::HARD_CLIP:
      out.left = ProcessHardClip(left_driven);
      out.right = ProcessHardClip(right_driven);
      break;

    case DistortionMode::SOFT_CLIP:
      out.left = ProcessSoftClip(left_driven);
      out.right = ProcessSoftClip(right_driven);
      break;

    case DistortionMode::TUBE:
      out.left = ProcessTube(left_driven);
      out.right = ProcessTube(right_driven);
      break;

    case DistortionMode::FOLDBACK:
      out.left = ProcessFoldback(left_driven);
      out.right = ProcessFoldback(right_driven);
      break;
  }

  return out;
}

void Distortion::SetParam(std::string name, double val) {
  if (name == "threshold") {
    SetThreshold(val);
  } else if (name == "drive") {
    SetDrive(val);
  } else if (name == "mode") {
    SetMode(static_cast<int>(val));
  }
}

void Distortion::SetThreshold(double val) {
  if (val >= 0.01 && val <= 1.0) {
    threshold_ = val;
  } else {
    printf("threshold must be between 0.01 and 1.0 (got %f)\n", val);
  }
}

void Distortion::SetDrive(double val) {
  if (val >= 1.0 && val <= 10.0) {
    drive_ = val;
  } else {
    printf("drive must be between 1.0 and 10.0 (got %f)\n", val);
  }
}

void Distortion::SetMode(int val) {
  if (val >= 0 && val <= 3) {
    mode_ = static_cast<DistortionMode>(val);
  } else {
    printf("mode must be 0-3 (0=HARD, 1=SOFT, 2=TUBE, 3=FOLDBACK) (got %d)\n",
           val);
  }
}

// ==================== DISTORTION ALGORITHMS ====================

double Distortion::ProcessHardClip(double input) {
  // Hard clipping - brick wall limiter
  double output;
  if (input >= 0) {
    output = fmin(input, threshold_);
  } else {
    output = fmax(input, -threshold_);
  }
  // Normalize by threshold to maintain consistent output level
  return output / threshold_;
}

double Distortion::ProcessSoftClip(double input) {
  // Soft clipping using tanh
  // Scale input by threshold, apply tanh, scale back
  double scaled = input / threshold_;
  double clipped = tanh(scaled);
  return clipped * 0.8;  // Scale down slightly to prevent loudness jump
}

double Distortion::ProcessTube(double input) {
  // Tube-style asymmetric saturation
  // Compresses positive peaks more than negative (mimics tube behavior)
  double output;

  if (input > 0) {
    // Positive: more compression (tubes compress positive more)
    double scaled = input / threshold_;
    output = threshold_ * (1.0 - exp(-scaled));
  } else {
    // Negative: less compression, more linear
    double scaled = -input / threshold_;
    output = -threshold_ * tanh(scaled * 0.7);
  }

  return output * 0.9;
}

double Distortion::ProcessFoldback(double input) {
  // Foldback/wavefold distortion
  // Signal "folds back" when it exceeds threshold
  double output = input;
  double fold_threshold = threshold_;

  // Keep folding while signal exceeds threshold
  int iterations = 0;
  const int max_iterations = 8;  // Prevent infinite loop

  while ((fabs(output) > fold_threshold) && (iterations < max_iterations)) {
    if (output > fold_threshold) {
      // Fold back from top
      output = 2.0 * fold_threshold - output;
    } else if (output < -fold_threshold) {
      // Fold back from bottom
      output = -2.0 * fold_threshold - output;
    }
    iterations++;
  }

  // Final hard clip in case we hit max iterations
  if (output > 1.0) output = 1.0;
  if (output < -1.0) output = -1.0;

  return output;
}
