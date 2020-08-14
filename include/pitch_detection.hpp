#pragma once

// Implementation of -
// http://www.cs.otago.ac.nz/tartini/papers/A_Smarter_Way_to_Find_Pitch.pdf
// code pinched from https://github.com/sevagh/pitch-detection

#include <complex>
#include <string>

#include <ffts/ffts.h>
#include <mlpack/core.hpp>
#include <mlpack/methods/hmm/hmm.hpp>

#define F0 440.0
#define N_BINS 108
#define N_NOTES 12
#define NOTE_OFFSET 57

// #define YIN_TRUST 0.5
#define TRANSITION_WIDTH 13
#define SELF_TRANS 0.99

#define MPM_CUTOFF 0.93
#define MPM_SMALL_CUTOFF 0.5
#define MPM_LOWER_PITCH_CUTOFF 80.0
#define PMPM_PA 0.01
#define PMPM_N_CUTOFFS 20
#define PMPM_PROB_DIST 0.05
#define PMPM_CUTOFF_BEGIN 0.8
#define PMPM_CUTOFF_STEP 0.01

const double Apitch = std::pow(2.0, 1.0 / 12.0);

int DetectMidiPitch(std::string sample_path);

std::vector<size_t>
BinPitches(const std::vector<std::pair<float, float>> pitch_candidates);

mlpack::hmm::HMM<mlpack::distribution::DiscreteDistribution> BuildHmm();

float PitchFromHmm(
    mlpack::hmm::HMM<mlpack::distribution::DiscreteDistribution> hmm,
    const std::vector<std::pair<float, float>> pitch_candidates);

class MPMPitchDetector
{
  public:
    long N_;
    std::vector<std::complex<float>> out_im_;
    std::vector<float> out_real_;
    ffts_plan_t *fft_forward_;
    ffts_plan_t *fft_backward_;
    mlpack::hmm::HMM<mlpack::distribution::DiscreteDistribution> hmm_;

    MPMPitchDetector(long audio_buffer_size);
    ~MPMPitchDetector();

    float GetPitch(const std::vector<float> &);
    void AutoCorrelate(const std::vector<float> &audio_buffer);

  private:
    void InitPitchBins();
    void Clear()
    {
        std::fill(out_im_.begin(), out_im_.end(),
                  std::complex<float>(0.0, 0.0));
    }
    std::vector<float> pitch_bins_;
    std::vector<float> real_pitches_;
};
