#pragma once

#include <dca.h>
#include <envelope_generator.h>
#include <filter_moogladder.h>
#include <qblimited_oscillator.h>
#include <soundgenerator.h>

class DrumSynth : public SoundGenerator
{
  public:
    DrumSynth();
    ~DrumSynth() = default;

    std::string Info() override;
    std::string Status() override;
    stereo_val genNext() override;
    void start() override;
    void noteOn(midi_event ev) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

    QBLimitedOscillator osc1;
    MoogLadder filter1;

    EnvelopeGenerator amp_env;
    EnvelopeGenerator mod_env;
    float mod_env_int{0.75};

    // QBLimitedOscillator osc2;
    // MoogLadder filter2;

    DCA m_dca;
};
