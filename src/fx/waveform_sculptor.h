#pragma once

#include <array>
#include <vector>

#include "fx.h"

// Landmark generation methods
enum class LandmarkStyle {
  kExtNCross,  // Extremities and zero-crossings
  kSpan,       // Spacing based on amplitude
  kDyDx,       // Based on derivative/slope
  kFreq,       // Regular frequency
  kRandom      // Random points
};

// Waveform recreation methods
enum class InterpStyle {
  kPolygon,    // Straight lines (lo-fi)
  kWrongygon,  // Backwards lines (harsh)
  kSing,       // Sine waves (tonal)
  kReversi,    // Reverse intervals
  kSmoothie,   // Smooth curves
  kPulse       // Just pulses
};

// Point operations
enum class PointOp {
  kNone,      // No operation
  kDouble,    // Double points
  kHalf,      // Thin by 1/2
  kLongpass,  // Remove close points
  kSlow,      // Time stretch
  kFast       // Time compress
};

// Window shapes for crossfading
enum class WindowShape { kTriangle, kCosine };

class WaveformSculptor : public Fx {
 public:
  WaveformSculptor();
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;

 private:
  void Init();
  void Reset();
  void SetInterpDefaults(InterpStyle style);

  // Windowing
  void ProcessWindow();
  float GetWindowScalar(int position, int window_size);

  // Core processing (operates on one window)
  int GenerateLandmarks(const float* input, int size, int* out_x, float* out_y,
                        int max_points);
  void ApplyOperation(int* x_points, float* y_points, int& num_points,
                      PointOp op, double param);
  void RecreateWaveform(const int* x_points, const float* y_points,
                        int num_points, const float* input, float* output,
                        int size);

  // Landmark generation implementations
  int GenerateExtNCross(const float* input, int size, int* out_x, float* out_y,
                        int max_points);
  int GenerateSpan(const float* input, int size, int* out_x, float* out_y,
                   int max_points);
  int GenerateDyDx(const float* input, int size, int* out_x, float* out_y,
                   int max_points);

  // Recreation implementations
  void RecreatePolygon(const int* x_points, const float* y_points,
                       int num_points, float* output, int size);
  void RecreateWrongygon(const int* x_points, const float* y_points,
                         int num_points, float* output, int size);
  void RecreateSing(const int* x_points, const float* y_points, int num_points,
                    const float* input, float* output, int size);
  void RecreateReversi(const int* x_points, const float* y_points,
                       int num_points, const float* input, float* output,
                       int size);

  // Parameters
  int window_size_;  // Window size in samples
  WindowShape window_shape_;
  LandmarkStyle landmark_style_;
  double landmark_param_;  // Parameter for landmark generation
  InterpStyle interp_style_;
  double interp_param_;  // Parameter for interpolation
  PointOp point_op_;
  double op_param_;
  double wet_mix_;  // 0.0 = dry, 1.0 = wet

  // Windowing state
  std::vector<float> input_buffer_;   // Accumulates input
  std::vector<float> output_buffer_;  // Accumulates output with crossfade
  std::vector<float> last_window_;    // Previous window for crossfade
  int samples_in_buffer_;
  int output_position_;  // Current read position in output buffer
  int hop_size_;         // How many samples to advance (window_size / 2)

  // Working buffers for landmark processing
  static constexpr int kMaxLandmarks = 512;
  std::array<int, kMaxLandmarks> temp_x_;
  std::array<float, kMaxLandmarks> temp_y_;

  // Debug info
  int last_num_landmarks_;

  // Constants
  static constexpr int kMinWindowSize = 256;
  static constexpr int kMaxWindowSize = 4096;
};
