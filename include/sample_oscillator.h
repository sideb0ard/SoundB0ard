#pragma once

#include "audiofile_data.h"
#include "oscillator.h"

enum
{
    SAMPLE_LOOP,
    SAMPLE_ONESHOT
};

struct SampleOscillator : public Oscillator
{
    SampleOscillator(std::string filename);
    ~SampleOscillator() = default;
    audiofile_data afd;
    int orig_pitch_midi_{36}; // default is C2

    bool is_single_cycle;
    bool is_pitchless;

    unsigned int loop_mode;

    double m_read_idx;

    void StartOscillator() override;
    void StopOscillator() override;
    void Reset() override;
    void Update() override;
    double DoOscillate(double *quad_phase_output) override;

    double ReadSampleBuffer();
};
