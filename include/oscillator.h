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
    unsigned m_mod_source_fo;
    unsigned m_mod_source_pulse_width;
    unsigned m_mod_source_amp;

    // destinations we write to
    unsigned m_mod_dest_output1;
    unsigned m_mod_dest_output2;

    bool just_wrapped; // true for one sample period. Used for hard sync
    bool m_note_on;
    // --- user controls or MIDI
    double m_osc_fo;    // oscillator frequency from MIDI note number
    double m_fo_ratio;  // FM Synth Modulator OR Hard Sync ratio
    double m_amplitude; // 0->1 from GUI

    // --- modulo counter and inc for timebase
    double m_modulo; // modulo counter 0->1
    double m_inc;    // phase inc = Freq/SR

    // --- more pitch mods
    int m_octave;       // octave tweak
    double m_semitones; // semitones tweak
    double m_cents;     // cents tweak

    // ---  pulse width in % (sqr only) from GUI
    double m_pulse_width_control;

    unsigned m_waveform;
    unsigned m_lfo_mode; // 0-2 LFOSYNC, LFOSHOT, LFOFREE

    unsigned m_midi_note_number;

    double m_fo;          // current (actual) frequency of oscillator
    double m_pulse_width; // pulse width in % for calculation
    // bool m_square_edge_rising; // hysteresis for square edge

    // --- for noise and random sample/hold
    unsigned m_pn_register; // for PN Noise sequence
    int m_rsh_counter;      // random sample/hold counter
    double m_rsh_value;     // currnet rsh output

    // --- for DPW
    double m_dpw_square_modulator; // square toggle
    double m_dpw_z1;               // memory register for differentiator

    // --- modulation inputs
    double m_fo_mod;         /* modulation input -1 to +1 */
    double m_pitch_bend_mod; /* modulation input -1 to +1 */
    double m_fo_mod_lin; /* FM modulation input -1 to +1 (not actually used in
                            Yamaha FM!) */
    double m_phase_mod;  /* Phase mod input -1 to +1 (used for DX synth) */
    double m_pw_mod;     /* modulation input for PWM -1 to +1 */
    double m_amp_mod; /* output amplitude modulation for AM 0 to +1 (not dB)*/

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
