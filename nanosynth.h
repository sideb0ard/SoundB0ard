#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "dca.h"
#include "envelope_generator.h"
#include "filter.h"
#include "keys.h"
#include "midimaaan.h"
#include "modmatrix.h"
#include "oscillator.h"
#include "sound_generator.h"

#define MAX_NUM_MIDI_LOOPS 16
#define MAX_VOICES 2

typedef enum { OSC, FILTER } LFO1_dest;
typedef enum { NONE, NANOSYNTH, DELAYFX } midi_control_type;

typedef midi_event *midi_events_loop_t[PPNS];

typedef struct nanosynth_voice {
    oscillator *osc1;
    oscillator *osc2;
    oscillator *lfo;
    envelope_generator *eg1;
    filter *f;
    dca *dca;

    modmatrix *g_modmatrix; // routing structure for sound generation

} nanosynth_voice;

typedef struct nanosynth {
    SOUNDGEN sound_generator;

    midi_events_loop_t melodies[MAX_NUM_MIDI_LOOPS];
    int melody_multiloop_count[MAX_NUM_MIDI_LOOPS];
    int num_melodies;
    int cur_melody;
    int cur_melody_iteration;
    bool multi_melody_mode;
    bool multi_melody_loop_countdown_started;

    int cur_octave;
    int sustain;

    bool recording;

    bool note_on;
    bool m_filter_keytrack;
    bool m_velocity_to_attack_scaling;
    bool m_note_number_to_decay_scaling;

    float vol;

    // global modmatrix, core is shared by all voices
    modmatrix *g_modmatrix; // routing structure for sound generation

    nanosynth_voice m_voices[MAX_VOICES];
    int m_pending_midi_note[MAX_VOICES];
    int m_pending_midi_velocity[MAX_VOICES];

    // need these for mod matrix
    double m_default_mod_range; // 1.0
    double m_osc_fo_mod_range;
    double m_filter_mod_range;
    double m_osc_fo_pitchbend_mod_range;
    double m_amp_mod_range;

    double m_default_mod_intensity; // 1.0
    double m_eg1_dca_intensity;
    double m_eg1_osc_intensity;
    double m_filter_keytrack_intensity;

    double last_val;

} nanosynth;

nanosynth *new_nanosynth(void);

// sound generator interface //////////////
double nanosynth_gennext(void *self);
// void nanosynth_gennext(void* self, double* frame_vals, int framesPerBuffer);
void nanosynth_status(void *self, wchar_t *status_string);
void nanosynth_setvol(void *self, double v);
double nanosynth_getvol(void *self);
////////////////////////////////////

void nanosynth_midi_note_on(nanosynth *self, unsigned int midinote,
                            unsigned int velocity);
bool nanosynth_midi_note_off(nanosynth *self, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off);
void nanosynth_midi_pitchbend(nanosynth *self, unsigned int data1,
                              unsigned int data2);
void nanosynth_midi_control(nanosynth *self, unsigned int data1,
                            unsigned int data2);
void nanosynth_start_note(nanosynth *self, int index, unsigned int midinote,
                          unsigned int velocity);
void nanosynth_steal_note(nanosynth *self, int index,
                          unsigned int pending_midinote,
                          unsigned int pending_velocity);

void change_octave(void *self, int direction);
void nanosynth_change_osc_wave_form(nanosynth *self, unsigned int voice_no,
                                    int oscil, bool all_voices);
void p_nanosynth_change_osc_wave_form(nanosynth *self, unsigned int voice_no,
                                      int oscil);
void nanosynth_set_sustain(nanosynth *self, int sustain_val);
void nanosynth_set_multi_melody_mode(nanosynth *self, bool melody_mode);
void nanosynth_set_melody_loop_num(nanosynth *self, int melody_num,
                                   int loop_num);
void nanosynth_add_melody(nanosynth *self);
void nanosynth_switch_melody(nanosynth *self, unsigned int melody_num);
void nanosynth_reset_melody(nanosynth *self, unsigned int melody_num);
void nanosynth_reset_melody_all(nanosynth *self);
void nanosynth_add_note(nanosynth *self, int midi_num);
void nanosynth_melody_to_string(nanosynth *self, int melody_num,
                                wchar_t scratch[33]);
