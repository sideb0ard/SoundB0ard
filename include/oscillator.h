#pragma once

#include <stdbool.h>

#include "modmatrix.h"
#include "synthfunctions.h"

#define OSC_FO_MOD_RANGE 2          // 2 semitone default
#define OSC_HARD_SYNC_RATIO_RANGE 4 // 4
#define OSC_PITCHBEND_MOD_RANGE 12  // 12 semitone default
#define OSC_FO_MIN 20               // 20Hz
#define OSC_FO_MAX 20480            // 20.480kHz = 10 octaves up from 20Hz
#define OSC_FO_DEFAULT 440.0        // A5
#define OSC_PULSEWIDTH_MIN 2        // 2%
#define OSC_PULSEWIDTH_MAX 98       // 98%
#define OSC_PULSEWIDTH_DEFAULT 50   // 50%

#define MIN_LFO_RATE 0.02
#define MAX_LFO_RATE 20.0
#define DEFAULT_LFO_RATE 0.5

// --- for PITCHED Oscillators
enum
{
    SINE,
    SAW1,
    SAW2,
    SAW3,
    TRI,
    SQUARE,
    NOISE,
    PNOISE,
    MAX_OSC
};

// --- for LFOs
enum
{
    sine,
    usaw,
    dsaw,
    tri,
    square,
    expo,
    rsh,
    qrsh,
    MAX_LFO_OSC
};

// --- for LFOs - MODE
enum
{
    LFOSYNC,
    LFOSHOT,
    LFORFREE,
    LFO_MAX_MODE
};

class Oscillator
{

  public:
    Oscillator();
    ~Oscillator() = default;

    // modulation matrix, owned by voice we are part of
    ModulationMatrix *modmatrix{nullptr};
    GlobalOscillatorParams *global_oscillator_params{nullptr};

    // sources that we read from
    unsigned m_mod_source_fo{DEST_NONE};
    unsigned m_mod_source_pulse_width{DEST_NONE};
    unsigned m_mod_source_amp{DEST_NONE};

    // destinations we write to
    unsigned m_mod_dest_output1{SOURCE_NONE};
    unsigned m_mod_dest_output2{SOURCE_NONE};

    bool just_wrapped{false}; // true for one sample period. Used for hard sync
    bool m_note_on{false};

    double m_osc_fo{OSC_FO_DEFAULT};
    double m_fo_ratio{1};  // FM Synth Modulator OR Hard Sync ratio
    double m_amplitude{1}; // default on

    double m_modulo{0}; // modulo counter 0->1
    double m_inc{0};    // phase inc = Freq/SR

    int m_octave{0};
    double m_semitones{0};
    double m_cents{0};

    double m_pulse_width_control{OSC_PULSEWIDTH_DEFAULT};

    unsigned m_waveform{SINE};
    unsigned m_lfo_mode{LFOSYNC};

    unsigned m_midi_note_number{0};

    double m_fo{OSC_FO_DEFAULT};
    double m_pulse_width{OSC_PULSEWIDTH_DEFAULT};
    // bool m_square_edge_rising; // hysteresis for square edge

    // --- for noise and random sample/hold
    unsigned m_pn_register{0}; // for PN Noise sequence
    int m_rsh_counter{-1};     // random sample hold
    double m_rsh_value{0};

    // --- for DPW
    double m_dpw_square_modulator{0}; // square toggle
    double m_dpw_z1{0};               // memory register for differentiator

    // --- modulation inputs
    double m_fo_mod{0};
    double m_pitch_bend_mod{0};
    double m_fo_mod_lin{0};
    double m_phase_mod{0};
    double m_pw_mod{0};
    double m_amp_mod{1};

    virtual double DoOscillate(double *aux_output) = 0;
    virtual void StartOscillator() = 0;
    virtual void StopOscillator() = 0;

    virtual void Reset();
    virtual void Update();

    void IncModulo();
    bool CheckWrapModulo();
    void ResetModulo(double d);
    void SetAmplitudeMod(double amp_val);
    void SetFoModExp(double fo_mod_val);
    void SetPitchBendMod(double mod_val);
    void SetFoModLin(double fo_mod_val);
    void SetPhaseMod(double mod_val);
    void SetPwMod(double mod_val);

    void InitGlobalParameters(GlobalOscillatorParams *params);
};
