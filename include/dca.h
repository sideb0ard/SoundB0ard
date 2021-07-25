#pragma once

#include "defjams.h"
#include "modmatrix.h"
#include "synthfunctions.h"

#define AMP_MOD_RANGE -96 // -96dB

struct DCA
{

    DCA() = default;
    ~DCA() = default;

    ModulationMatrix *modmatrix{nullptr};
    GlobalDCAParams *global_dca_params{nullptr};

    unsigned m_mod_source_eg{DEST_NONE};
    unsigned m_mod_source_amp_db{DEST_NONE};
    unsigned m_mod_source_velocity{DEST_NONE};
    unsigned m_mod_source_pan{DEST_NONE};

    double m_gain{1};

    unsigned int m_midi_velocity{127};

    double m_amplitude_db{0};
    double m_amplitude_control{1};

    double m_amp_mod_db{0};
    double m_eg_mod{1};
    double m_pan_control{0};
    double m_pan_mod{0};

    //// funcz

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
