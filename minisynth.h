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
#include "stereodelay.h"

#include "minisynth_voice.h"

#define MAX_NUM_MIDI_LOOPS 16
#define MAX_VOICES 4

#define MIN_DETUNE_CENTS -50.0
#define MAX_DETUNE_CENTS 50.0
#define DEFAULT_DETUNE_CENTS 0.0

#define MIN_PULSE_WIDTH_PCT 1.0
#define MAX_PULSE_WIDTH_PCT 99.0
#define DEFAULT_PULSE_WIDTH_PCT 50.0
#define MIN_NOISE_OSC_AMP_DB -96.0
#define MAX_NOISE_OSC_AMP_DB 0.0
#define DEFAULT_NOISE_OSC_AMP_DB -96.0
#define MIN_SUB_OSC_AMP_DB -96.0
#define MAX_SUB_OSC_AMP_DB 0.0
#define DEFAULT_SUB_OSC_AMP_DB -96.0

#define DEFAULT_LEGATO_MODE 0
#define DEFAULT_RESET_TO_ZERO 0
#define DEFAULT_FILTER_KEYTRACK 0
#define DEFAULT_FILTER_KEYTRACK_INTENSITY 0.5
#define DEFAULT_VELOCITY_TO_ATTACK 0
#define DEFAULT_NOTE_TO_DECAY 0
#define DEFAULT_MIDI_PITCHBEND 0
#define DEFAULT_MIDI_MODWHEEL 0
#define DEFAULT_MIDI_VOLUME 127
#define DEFAULT_MIDI_PAN 64
#define DEFAULT_MIDI_EXPRESSION 0
#define DEFAULT_PORTAMENTO_TIME_MSEC 0.0

typedef struct minisynth {
    SOUNDGEN sound_generator;

    midi_events_loop_t melodies[MAX_NUM_MIDI_LOOPS];
    int melody_multiloop_count[MAX_NUM_MIDI_LOOPS];

    int num_melodies;
    int cur_melody;
    int cur_melody_iteration;

    bool multi_melody_mode;
    bool multi_melody_loop_countdown_started;

    int sustain;

    bool recording;
    unsigned int m_midi_knob_mode; // midi routings, 1..3

    //float vol;

    // end SOUNDGEN stuff

    minisynth_voice *m_voices[MAX_VOICES];

    // global modmatrix, core is shared by all voices
    modmatrix m_ms_modmatrix; // routing structure for sound generation
    global_synth_params m_global_synth_params;

    double m_last_note_frequency;

    unsigned int m_midi_rx_channel;

    stereodelay m_delay_fx;

    unsigned int m_voice_mode; // controlled by keys
    // midi mode one, top row
    double m_attack_time_msec;
    double m_decay_release_time_msec;
    double m_sustain_level;
    double m_volume_db;
    // midi mode one, bottom row
    double m_lfo1_amplitude;
    double m_lfo1_rate;
    double m_fc_control;
    double m_q_control;

    // midi mode two, top row
    double m_delay_time_msec;
    double m_feedback_pct;
    double m_delay_ratio;
    double m_wet_mix;
    unsigned int m_delay_mode; // via keyboard 'n' key (TODO!)
    // midi mode two, bottom row
    double m_detune_cents;
    double m_pulse_width_pct;
    double m_sub_osc_db;
    double m_noise_osc_db;

    // midi mode three, top row
    double m_eg1_dca_intensity;
    double m_eg1_filter_intensity;
    double m_eg1_osc_intensity;
    double m_filter_keytrack_intensity;
    // midi mode three, bottom row
    double m_lfo1_amp_intensity;
    double m_lfo1_filter_fc_intensity;
    double m_lfo1_osc_pitch_intensity;
    double m_lfo1_pan_intensity;

    int m_octave;
    int m_pitchbend_range;

    unsigned int m_lfo1_waveform;
    unsigned int m_legato_mode;
    unsigned int m_reset_to_zero;
    unsigned int m_filter_keytrack;
    unsigned int m_velocity_to_attack_scaling;
    unsigned int m_note_number_to_decay_scaling;

    double m_portamento_time_msec;
    bool m_sustain_override;

} minisynth;

minisynth *new_minisynth(void);

// sound generator interface //////////////
double minisynth_gennext(void *self);
// void minisynth_gennext(void* self, double* frame_vals, int framesPerBuffer);
void minisynth_status(void *self, wchar_t *status_string);
void minisynth_setvol(void *self, double v);
double minisynth_getvol(void *self);
////////////////////////////////////

// Will Pirkle model
bool minisynth_prepare_for_play(minisynth *synth);
void minisynth_user_interface_change(minisynth *synth);

void minisynth_update(minisynth *synth);

void minisynth_increment_voice_timestamps(minisynth *synth);
minisynth_voice *minisynth_get_oldest_voice(minisynth *synth);
minisynth_voice *minisynth_get_oldest_voice_with_note(minisynth *synth,
                                                      unsigned int midi_note);

bool minisynth_midi_note_on(minisynth *self, unsigned int midinote,
                            unsigned int velocity);
bool minisynth_midi_note_off(minisynth *self, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off);
void minisynth_midi_mod_wheel(minisynth *self, unsigned int data1,
                              unsigned int data2);
void minisynth_midi_pitchbend(minisynth *self, unsigned int data1,
                              unsigned int data2);
void minisynth_midi_control(minisynth *self, unsigned int data1,
                            unsigned int data2);

void minisynth_set_sustain(minisynth *self, int sustain_val);
void minisynth_set_multi_melody_mode(minisynth *self, bool melody_mode);
void minisynth_set_melody_loop_num(minisynth *self, int melody_num,
                                   int loop_num);
void minisynth_add_melody(minisynth *self);
void minisynth_dupe_melody(minisynth *self);
void minisynth_switch_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody_all(minisynth *self);
void minisynth_reset_voices(minisynth *self);
void minisynth_melody_to_string(minisynth *self, int melody_num,
                                wchar_t scratch[33]);
midi_event **minisynth_get_midi_loop(minisynth *self);
void minisynth_add_event(minisynth *self, midi_event *ev);
midi_event **minisynth_copy_midi_loop(minisynth *self, int pattern_num);
void minisynth_add_midi_loop(minisynth *self, midi_event **events,
                             int pattern_num);
void minisynth_toggle_delay_mode(minisynth *ms);
void minisynth_set_sustain_override(minisynth *ms, bool b);
