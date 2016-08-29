#pragma once

#include <stdbool.h>

#include "modmatrix.h"
#include "sound_generator.h"

#define OSC_FO_MOD_RANGE 2          // 2 semitone default
#define OSC_HARD_SYNC_RATIO_RANGE 4 // 4
#define OSC_PITCHBEND_MOD_RANGE 12  // 12 semitone default
#define OSC_FO_MIN 20               // 20Hz
#define OSC_FO_MAX 20480            // 20.480kHz = 10 octaves up from 20Hz
#define OSC_FO_DEFAULT 440.0        // A5
#define OSC_PULSEWIDTH_MIN 2        // 2%
#define OSC_PULSEWIDTH_MAX 98       // 98%
#define OSC_PULSEWIDTH_DEFAULT 50   // 50%

#define MIN_LFO_RATE -0.02
#define MAX_LFO_RATE 20.0
#define DEFAULT_LFO_RATE 0.5

typedef struct oscillator oscillator;

struct oscillator {

    // --- render a sample
    // for LFO:  pAuxOutput = QuadPhaseOutput
    // Pitched: pAuxOutput = Right channel (return value is left
    // Channel
    double (*do_oscillate)(oscillator *self, double *aux_output);
    //// TODO: implement these per subclass
    void (*start_oscillator)(oscillator *self);
    void (*stop_oscillator)(oscillator *self);
    void (*reset_oscillator)(oscillator *self);
    void (*update_oscillator)(oscillator *self);

    // global modulation matrix
    modmatrix *g_modmatrix;
    // sources that we read from
    unsigned m_mod_source_fo;
    unsigned m_mod_source_pulse_width;
    unsigned m_mod_source_amp;
    // destinations we write to
    unsigned m_mod_dest_output1;
    unsigned m_mod_dest_output2;

    bool m_note_on;
    // --- user controls or MIDI
    double m_osc_fo;    // oscillator frequency from MIDI note number
    double m_fo_ratio;  // FM Synth Modulator OR Hard Sync ratio
    double m_amplitude; // 0->1 from GUI

    // --- modulo counter and inc for timebase
    double m_modulo; // modulo counter 0->1
    double m_inc;    // phase inc = fo/fs

    // --- more pitch mods
    double m_octave;    // octave tweak
    double m_semitones; // semitones tweak
    double m_cents;     // cents tweak

    // ---  pulse width in % (sqr only) from GUI
    double m_pulse_width_control;

    // --- for PITCHED Oscillators
    enum { SINE, SAW1, SAW2, SAW3, TRI, SQUARE, NOISE, PNOISE, MAX_OSC };
    unsigned m_waveform; // to store type

    // --- for LFOs
    enum { sine, usaw, dsaw, tri, square, expo, rsh, qrsh, MAX_LFO_OSC };

    // --- for LFOs - MODE
    enum { sync, shot, rfree };
    unsigned m_lfo_mode; // to store MODE

    // --- for hard sync or other dual-oscillator ideas
    oscillator *m_buddy_oscillator;

    // --- flag indicating we are a master oscillator
    bool m_master_osc;

    // --- MIDI note that is being played
    unsigned m_midi_note_number;

    // --- PROTECTED: generally these are either basic calc variables
    //                and modulation stuff
    // --- calculation variables
    // double m_sample_rate; // fs
    double m_fo;               // current (actual) frequency of oscillator
    double m_pulse_width;      // pulse width in % for calculation
    bool m_square_edge_rising; // hysteresis for square edge

    // --- for noise and random sample/hold
    unsigned m_pn_register; // for PN Noise sequence
    int m_rsh_counter;      // random sample/hold counter
    double m_rsh_value;     // currnet rsh output

    // --- for DPW
    double m_dpw_square_modulator; // square toggle
    double m_dpw_z1;               // memory register for differentiator

    // --- mondulation inputs
    double m_fo_mod;         /* modulation input -1 to +1 */
    double m_pitch_bend_mod; /* modulation input -1 to +1 */
    double m_fo_mod_lin; /* FM modulation input -1 to +1 (not actually used in
                            Yamaha FM!) */
    double m_phase_mod;  /* Phase mod input -1 to +1 (used for DX synth) */
    double m_pw_mod;     /* modulation input for PWM -1 to +1 */
    double m_amp_mod; /* output amplitude modulation for AM 0 to +1 (not dB)*/
};

void osc_new_settings(oscillator *self);

// --- modulo functions for master/slave operation
// --- increment the modulo counters
void osc_inc_modulo(oscillator *self);

void osc_set_fo_mod_exp(oscillator *self, double fo_mod_val);

// --- check and wrap the modulo
//     returns true if modulo wrapped
bool osc_check_wrap_modulo(oscillator *self);

// --- reset the modulo (required for master->slave operations)
void osc_reset_modulo(oscillator *self, double d);

// --- reset counters, etc...
void osc_reset(oscillator *self);
// --- update the frequency, amp mod and PWM
void osc_update(oscillator *self);
