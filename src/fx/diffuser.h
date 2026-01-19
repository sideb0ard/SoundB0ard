#pragma once

#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include "fx.h"

class Diffuser : public Fx {
 public:
  struct Parameters {
    double wet = 0.5f;
    double wet_gain = 1.0f;
    double size = 0.5f;
    double feedback = 0.3f;
    double unison = 0.0f;
    double diffuse[4] = {0.0f, 0.1f, 0.1f, 0.0f};
    double inertia[4] = {0.1f, 0.1f, 0.1f, 0.1f};
    // double wet = 1.0;
    // double wet_gain = 1.5;
    // double size = 0.8f;
    // double feedback = 0.7f;
    // double unison = 0.5f;
    // double diffuse[4] = {0.6f, 0.4f, 0.0f, 0.0f};
    // double inertia[4] = {0.1f, 0.1f, 0.1f, 0.1f};
  };

  Diffuser();
  ~Diffuser() = default;
  std::string Status() override;
  StereoVal Process(StereoVal input) override;
  void SetParam(std::string name, double val) override;
  void EventNotify(broadcast_event event, mixer_timing_info tinfo) override;

  void SetParameters(const Parameters& params);
  void Reset();

 private:
  // Constants
  static constexpr int kMaxKernelSize = 7;
  static constexpr int kMaxDelayMs = 1000;
  static constexpr int kMaxDelayTaps = 72;
  static constexpr int kMaxUnisonVoices = 8;
  static constexpr double kSmoothingFast = 0.001f;
  static constexpr double kSmoothingSlow = 0.0001f;

  // Core delay line
  class DelayLine {
   public:
    explicit DelayLine(int max_samples);
    void Write(double sample);
    double Read(int delay_samples) const;
    double ReadInterpolated(double delay_samples) const;
    void Clear();

   private:
    std::vector<double> buffer_;
    int write_index_ = 0;
    int max_samples_;
  };

  // All-pass filter for diffusion
  class AllPassFilter {
   public:
    AllPassFilter(int delay_samples, double feedback);
    double Process(double input);
    void SetFeedback(double feedback) {
      feedback_ = feedback;
    }
    void Clear();

   private:
    DelayLine delay_;
    double feedback_;
    int delay_samples_;
  };

  // Diffusion stage with multiple all-pass filters
  class DiffusionStage {
   public:
    explicit DiffusionStage(int sample_rate);
    StereoVal Process(StereoVal input, double amount);
    void Clear();

   private:
    static constexpr int kNumAllPasses = 4;
    std::vector<AllPassFilter> left_apfs_;
    std::vector<AllPassFilter> right_apfs_;
    double last_amount_ = 0.0f;
  };

  // Parameter smoothing with inertia
  class SmoothParameter {
   public:
    void SetTarget(double target) {
      target_ = target;
    }
    void SetInertia(double inertia);
    double Process();
    double GetCurrent() const {
      return current_;
    }
    void Reset(double value);

   private:
    double current_ = 0.0f;
    double target_ = 0.0f;
    double coefficient_ = kSmoothingFast;
  };

  // Delay tap structure
  struct DelayTap {
    int delay_samples;
    double feedback;
    double cross_feed;
  };

  // Unison voice structure
  struct UnisonVoice {
    double detune_ratio;
    double pan_left;
    double pan_right;
    int delay_offset;
    mutable double phase;
  };

  // Processing methods
  void UpdateParameters();
  void UpdateSize(double size);
  void UpdateUnison(double unison);
  StereoVal ProcessBlur(StereoVal input);
  StereoVal ProcessUnison(StereoVal input);
  StereoVal ProcessDiffusion(StereoVal input);
  StereoVal ApplyFeedback(StereoVal input);

  // Utility methods
  int MsToSamples(double ms) const;
  double SamplesToMs(int samples) const;
  int WrapIndex(int index, int buffer_size) const;

  // Member variables
  int sample_rate_{SAMPLE_RATE};
  int max_delay_samples_{0};
  Parameters params_;
  Parameters target_params_;

  // Delay buffers
  std::array<std::unique_ptr<DelayLine>, 2> main_delays_;
  std::array<std::unique_ptr<DelayLine>, 2> feedback_delays_;

  // Blur kernel
  int kernel_size_ = 3;
  std::vector<std::vector<double>> kernel_;

  // Delay taps
  std::vector<DelayTap> delay_taps_;
  int active_taps_ = 12;

  // Unison voices
  std::vector<UnisonVoice> unison_voices_;
  std::array<double, kMaxUnisonVoices> unison_phases_;

  // Diffusion stages
  std::array<std::unique_ptr<DiffusionStage>, 4> diffusion_stages_;

  // Feedback buffer
  StereoVal feedback_buffer_{0.0, 0.0};

  // Parameter smoothing
  SmoothParameter smooth_wet_;
  SmoothParameter smooth_wet_gain_;
  SmoothParameter smooth_size_;
  SmoothParameter smooth_feedback_;
  SmoothParameter smooth_unison_;
  std::array<SmoothParameter, 4> smooth_diffuse_;
  std::array<SmoothParameter, 4> inertia_processors_;

  // Modulation
  double lfo_phase_ = 0.0f;
  double lfo_rate_ = 0.3f;  // Hz
};
