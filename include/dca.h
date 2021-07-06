#pragma once

#include "defjams.h"
#include "modmatrix.h"
#include "synthfunctions.h"

#define AMP_MOD_RANGE -96 // -96dB

struct DCA
{

    DCA();
    ~DCA() = default;

    ModulationMatrix *modmatrix{nullptr};
    GlobalDCAParams *global_dca_params{nullptr};

    unsigned m_mod_source_eg;
    unsigned m_mod_source_amp_db;
    unsigned m_mod_source_velocity;
    unsigned m_mod_source_pan;

    double m_gain;

    unsigned int m_midi_velocity;

    double m_amplitude_db; // gui?
    double m_amplitude_control;

    double m_pan_control;
    double m_amp_mod_db;
    double m_eg_mod;
    double m_pan_mod;

    void SetPanControl(double pan);
    void Reset();
    void SetAmplitudeDb(double amp);
    void SetAmpModDb(double mod);
    void SetMidiVelocity(unsigned int vel);
    void SetEgMod(double mod);
    void SetPanMod(double mod);
    void Update();
    void DoDCA(double left_input, double right_input, double *left_output,
               double *right_output);
    void InitGlobalParameters(GlobalDCAParams *params);
};
