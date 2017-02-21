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

#include "minisynth_voice.h"

#define MAX_NUM_MIDI_LOOPS 16
#define MAX_VOICES 16

typedef struct minisynth {
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

    float vol;

    // end SOUNDGEN stuff

    minisynth_voice *m_voices[MAX_VOICES];

    // global modmatrix, core is shared by all voices
    modmatrix m_ms_modmatrix; // routing structure for sound generation
    global_synth_params m_global_synth_params;

    double m_last_note_frequency;

    unsigned int m_midi_rx_channel;

    // new delay! fx! TODO
    // stereodelay fx m_delay_fx;

    unsigned int m_voice_mode;
    double m_detune_cents;
    double m_fc_control;
    double m_lfo1_rate;
    double m_attack_time_msec;
    double m_pulse_width_pct;
    double m_delay_time_msec;
    double m_feedback_pct;
    double m_delay_ratio;
    double m_wet_mix;
    int m_octave;
    double m_portamento_time_msec;
    double m_q_control;
    double m_lfo1_osc_pitch_intensity;
    double m_decay_release_time_msec;
    double m_lfo1_amplitude;
    double m_sub_osc_db;
    double m_eg1_osc_intensity;
    double m_eg1_filter_intensity;
    double m_lfo1_filter_fc_intensity;
    double m_sustain_level;
    double m_noise_osc_db;
    double m_lfo1_amp_intensity;
    double m_lfo1_pan_intensity;
    unsigned int m_lfo1_waveform;
    double m_eg1_dca_intensity;
    double m_volume_db;
    unsigned int m_legato_mode;
    int m_pitchbend_range;
    unsigned int m_reset_to_zero;
    unsigned int m_filter_keytrack;
    double m_filter_keytrack_intensity;
    unsigned int m_velocity_to_attack_scaling;
    unsigned int m_note_number_to_decay_scaling;
    unsigned int m_delaymode;

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
void minisynth_switch_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody_all(minisynth *self);
void minisynth_melody_to_string(minisynth *self, int melody_num,
                                wchar_t scratch[33]);
