#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "sound_generator.h"
#include "table.h"

#define OSC_FQ_MOD_RANGE 2
#define OSC_PITCHBEND_MOD_RANGE 12
#define OSC_FQ_MIN 2
#define OSC_FQ_MAX 20480
#define OSC_FQ_DEFAULT 440.0
#define OSC_PULSEWIDTH_MIN 2
#define OSC_PULSEWIDTH_MAX 98
#define OSC_PULSEWIDTH_DEFAULT 50

#define MIN_LFO_RATE 0.02
#define MAX_LFO_RATE 10.0
#define DEFAULT_LFO_RATE 0.5

typedef struct oscil OSCIL;

typedef void (*freqy)(OSCIL *osc, double freq);
typedef void (*incry)(OSCIL *osc, double freq);

struct oscil {

    SOUNDGEN sound_generator;

    // "public"
    double freq; // m_dOscFo
    double m_fq;
    double m_fq_ratio;
    double m_amplitude;
    double m_pw_control;
    double curphase; // m_dModulo
    double incr;     // m_dInc

    int m_octave;
    double m_semitones;
    double m_cents;

    bool m_note_on;
    double vol;
    double m_amp;

    wave_type wav;  // defined in defjams - NOISE, SAW etc.
    wtable *wtable; // m_uWaveform
    double dtablen;

    unsigned m_lfo_mode;
    unsigned m_midi_note_number;

    // "private"
    double m_output_val; // current oscilator freq including all modulations
    double m_pulse_width;
    double m_rsh_value; // for random sample and hold

    double m_dpw_square_modulator; // no idea
    double m_dpw_z1;               // ditto
    double m_fq_mod_exp;           // frequency modulation exponential
    double m_fq_mod_lin;           // frequency modulation linear
    double m_pitch_bend_mod;       // -1 .. +1
    double m_phase_mod;            // radians
    double m_pw_mod;               // pulse width mod -1 .. +1
    double m_amp_mod;              // amplitude 0.. +1 ( not tremelo)
    double m_pn_register; // register for quasi random noise generator? wtf?
    int m_rsh_counter;    // counter for random sample and hold output

    freqy freqadj;
    incry incradj;
};

OSCIL *new_oscil(double freq, wave_type t);
void osc_set_wave(OSCIL *self, wave_type t);

// void oscil_gennext(void* self, double* frame_vals, int framesPerBuffer);
void oscil_setvol(void *self, double v);
double oscil_getvol(void *self);
void set_freq(OSCIL *p_osc, double freq);
void incrfunc(OSCIL *p_osc, double v);

void set_midi_note_num(OSCIL *self, int midi_note_num);
void pitch_bend(OSCIL *self, double cents);
void reset_modulo(OSCIL *self);
void set_amp_modulo(OSCIL *self);
void set_fq_mod_exp(OSCIL *self, double mod);
void set_fq_mod_lin(OSCIL *self, double mod);
void set_pitch_bend_mod(OSCIL *self, double bend);
void set_phase_mod(OSCIL *self, double phase);
void set_pw_mod(OSCIL *self, double pw);

double oscil_gennext(void *self);

void osc_start(OSCIL *self);
void osc_stop(OSCIL *self);

void osc_reset(OSCIL *self);
void osc_update(OSCIL *self);

void oscil_status(void *self, char *status_string);
void freqinc(OSCIL *p_osc, int direction);
