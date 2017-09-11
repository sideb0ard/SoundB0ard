#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "arpeggiator.h"
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

#define MAX_NUM_MIDI_LOOPS 64
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

static const char PRESET_FILENAME[] = "settings/synthpresets.dat";

typedef struct synthsettings {
    char m_settings_name[256];

    unsigned int m_voice_mode;

    unsigned int m_lfo1_waveform;
    double m_lfo1_rate;
    double m_lfo1_amplitude;
    double m_lfo1_amp_intensity;
    double m_lfo1_filter_fc_intensity;
    double m_lfo1_osc_pitch_intensity;
    double m_lfo1_pan_intensity;

    unsigned int m_lfo2_waveform;
    double m_lfo2_rate;
    double m_lfo2_amplitude;
    double m_lfo2_amp_intensity;
    double m_lfo2_filter_fc_intensity;
    double m_lfo2_osc_pitch_intensity;
    double m_lfo2_pan_intensity;

    double m_attack_time_msec;
    double m_decay_time_msec;
    double m_release_time_msec;
    double m_sustain_level;

    double m_volume_db;
    double m_fc_control;
    double m_q_control;

    double m_delay_time_msec;
    double m_delay_feedback_pct;
    double m_delay_ratio;
    double m_delay_wet_mix;
    unsigned int m_delay_mode;

    double m_detune_cents;
    double m_pulse_width_pct;
    double m_sub_osc_db;
    double m_noise_osc_db;
    double m_eg1_dca_intensity;
    double m_eg1_filter_intensity;
    double m_eg1_osc_intensity;
    double m_filter_keytrack_intensity;

    int m_octave;
    int m_pitchbend_range;
    unsigned int m_legato_mode;
    unsigned int m_reset_to_zero;
    unsigned int m_filter_keytrack;
    unsigned int m_filter_type;
    double m_filter_saturation;
    unsigned int m_nlp;
    unsigned int m_velocity_to_attack_scaling;
    unsigned int m_note_number_to_decay_scaling;
    double m_portamento_time_msec;
    unsigned int m_sustain_override;
    double m_sustain_time_ms;
    double m_sustain_time_sixteenth;
} synthsettings;

typedef struct minisynth {
    SOUNDGEN sound_generator;

    int tick; // current 16th note tick from mixer
    midi_events_loop_t melodies[MAX_NUM_MIDI_LOOPS];
    int melody_multiloop_count[MAX_NUM_MIDI_LOOPS];
    midi_events_loop_t backup_melody_while_getting_crazy;

    int num_melodies;
    int cur_melody;
    int cur_melody_iteration;

    bool multi_melody_mode;
    bool multi_melody_loop_countdown_started;

    bool active;

    bool recording;
    unsigned int m_midi_knob_mode; // midi routings, 1..3

    minisynth_voice *m_voices[MAX_VOICES];

    // global modmatrix, core is shared by all voices
    modmatrix m_ms_modmatrix; // routing structure for sound generation
    global_synth_params m_global_synth_params;

    double m_last_note_frequency;

    unsigned int m_midi_rx_channel;

    stereodelay m_delay_fx;

    synthsettings m_settings;
    synthsettings m_settings_backup_while_getting_crazy;

    bool morph_mode; // magical
    int morph_every_n_loops;
    int morph_generation;

    bool generate_mode; // magical
    int generate_every_n_loops;
    int generate_generation;

    int max_generation;

    int m_last_midi_note;
    arpeggiator m_arp;

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

void minisynth_clear_melody_ready_for_new_one(minisynth *ms, int melody_num);
void minisynth_stop(minisynth *ms);

void minisynth_update(minisynth *synth);

void minisynth_generate_melody(minisynth *ms, int melody_num, int max_notes,
                               int max_steps);

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

void minisynth_set_multi_melody_mode(minisynth *self, bool melody_mode);
void minisynth_set_melody_loop_num(minisynth *self, int melody_num,
                                   int loop_num);
int minisynth_add_melody(minisynth *self);
void minisynth_dupe_melody(midi_event **from, midi_event **to);
void minisynth_switch_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody(minisynth *self, unsigned int melody_num);
void minisynth_reset_melody_all(minisynth *self);
void minisynth_reset_voices(minisynth *self);
void minisynth_melody_to_string(minisynth *self, int melody_num,
                                wchar_t scratch[33]);
int minisynth_add_event(minisynth *self, int pattern_num, midi_event *ev);
void minisynth_delete_event(minisynth *ms, int pat_num, int tick);

midi_event **minisynth_get_midi_loop(minisynth *self);
midi_event **minisynth_copy_midi_loop(minisynth *self, int pattern_num);
void minisynth_add_midi_loop(minisynth *self, midi_event **events,
                             int pattern_num);
void minisynth_replace_midi_loop(minisynth *ms, midi_event **events,
                                 int melody_num);
void minisynth_toggle_delay_mode(minisynth *ms);

void minisynth_rand_settings(minisynth *ms);
void minisynth_print_melodies(minisynth *ms);
void minisynth_print_settings(minisynth *ms);
bool minisynth_save_settings(minisynth *ms, char *preset_name);
bool minisynth_load_settings(minisynth *ms, char *preset_name);
bool minisynth_list_presets(void);
bool minisynth_check_if_preset_exists(char *preset_to_find);

void minisynth_nudge_melody(minisynth *ms, int melody_num, int sixteenth);
bool is_valid_melody_num(minisynth *ns, int melody_num);

void minisynth_handle_midi_note(minisynth *ms, int note, int velocity,
                                bool update_last_midi);
void minisynth_set_arpeggiate(minisynth *ms, bool b);
void minisynth_set_arpeggiate_latch(minisynth *ms, bool b);
void minisynth_set_arpeggiate_single_note_repeat(minisynth *ms, bool b);
void minisynth_set_arpeggiate_octave_range(minisynth *ms, int val);
void minisynth_set_arpeggiate_mode(minisynth *ms, unsigned int mode);
void minisynth_set_arpeggiate_rate(minisynth *ms, unsigned int mode);

void minisynth_import_midi_from_file(minisynth *ms, char *filename);
void minisynth_set_filter_mod(minisynth *ms, double mod);
void minisynth_del_self(minisynth *ms);

void minisynth_set_generate_mode(minisynth *ms, bool b);
void minisynth_set_morph_mode(minisynth *ms, bool b);
void minisynth_set_backup_mode(minisynth *ms, bool b);
void minisynth_morph(minisynth *ms);
int minisynth_get_notes_from_melody(midi_event **melody,
                                    int return_midi_notes[10]);

void minisynth_sg_start(void *self);
void minisynth_sg_stop(void *self);

int minisynth_get_num_tracks(void *self);
int minisynth_get_num_notes(minisynth *ms);
void minisynth_make_active_track(void *self, int pattern_num);

void minisynth_print(minisynth *ms);
void minisynth_add_note(minisynth *ms, int pattern_num, int step,
                        int midi_note);
void minisynth_rm_note(minisynth *ms, int pattern_num, int step);
void minisynth_mv_note(minisynth *ms, int pattern_num, int fromstep,
                       int tostep);
void minisynth_add_micro_note(minisynth *ms, int pattern_num, int step,
                              int midi_note);
void minisynth_rm_micro_note(minisynth *ms, int pattern_num, int step);
void minisynth_mv_micro_note(minisynth *ms, int pattern_num, int fromstep,
                             int tostep);

void minisynth_set_attack_time_ms(minisynth *ms, double val);
void minisynth_set_decay_time_ms(minisynth *ms, double val);
void minisynth_set_release_time_ms(minisynth *ms, double val);
void minisynth_set_delay_feedback_pct(minisynth *ms, double val);
void minisynth_set_delay_ratio(minisynth *ms, double val);
void minisynth_set_delay_mode(minisynth *ms, unsigned int val);
void minisynth_set_delay_time_ms(minisynth *ms, double val);
void minisynth_set_delay_wetmix(minisynth *ms, double val);
void minisynth_set_detune(minisynth *ms, double val);
void minisynth_set_eg1_dca_int(minisynth *ms, double val);
void minisynth_set_eg1_filter_int(minisynth *ms, double val);
void minisynth_set_eg1_osc_int(minisynth *ms, double val);
void minisynth_set_filter_fc(minisynth *ms, double val);
void minisynth_set_filter_fq(minisynth *ms, double val);
void minisynth_set_filter_type(minisynth *ms, unsigned int val);
void minisynth_set_filter_saturation(minisynth *ms, double val);
void minisynth_set_filter_nlp(minisynth *ms, unsigned int val);
void minisynth_set_keytrack_int(minisynth *ms, double val);
void minisynth_set_keytrack(minisynth *ms, unsigned int val);
void minisynth_set_legato_mode(minisynth *ms, unsigned int val);
// LFO
void minisynth_set_lfo_amp_int(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_amp(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_filter_fc_int(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_rate(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_pan_int(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_pitch(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_wave(minisynth *ms, int lfo_num, unsigned int val);

void minisynth_set_note_to_decay_scaling(minisynth *ms, unsigned int val);
void minisynth_set_noise_osc_db(minisynth *ms, double val);
void minisynth_set_octave(minisynth *ms, int val);
void minisynth_set_pitchbend_range(minisynth *ms, int val);
void minisynth_set_portamento_time_ms(minisynth *ms, double val);
void minisynth_set_pulsewidth_pct(minisynth *ms, double val);
void minisynth_set_sub_osc_db(minisynth *ms, double val);
void minisynth_set_sustain(minisynth *ms, double val);
void minisynth_set_sustain_time_ms(minisynth *ms, double val);
void minisynth_set_sustain_time_sixteenth(minisynth *ms, double val);
void minisynth_set_sustain_override(minisynth *ms, bool b);
void minisynth_set_velocity_to_attack_scaling(minisynth *ms, unsigned int val);
void minisynth_set_voice_mode(minisynth *ms, unsigned int val);
void minisynth_set_vol(minisynth *ms, double val);
void minisynth_set_reset_to_zero(minisynth *ms, unsigned int val);
