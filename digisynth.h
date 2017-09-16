#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "digisynth_voice.h"

#define MAX_VOICES 3

typedef struct digisynth
{
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

    digisynth_voice *m_voices[MAX_VOICES];

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

} digisynth;

digisynth *new_digisynth(void);

// sound generator interface //////////////
double digisynth_gennext(void *self);
// void digisynth_gennext(void* self, double* frame_vals, int framesPerBuffer);
void digisynth_status(void *self, wchar_t *status_string);
void digisynth_setvol(void *self, double v);
double digisynth_getvol(void *self);
////////////////////////////////////

// Will Pirkle model
bool digisynth_prepare_for_play(digisynth *synth);

void digisynth_clear_melody_ready_for_new_one(digisynth *ms, int melody_num);
void digisynth_stop(digisynth *ms);

void digisynth_update(digisynth *synth);

void digisynth_generate_melody(digisynth *ms, int melody_num, int max_notes,
                               int max_steps);

void digisynth_increment_voice_timestamps(digisynth *synth);
digisynth_voice *digisynth_get_oldest_voice(digisynth *synth);
digisynth_voice *digisynth_get_oldest_voice_with_note(digisynth *synth,
                                                      unsigned int midi_note);

bool digisynth_midi_note_on(digisynth *self, unsigned int midinote,
                            unsigned int velocity);
bool digisynth_midi_note_off(digisynth *self, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off);
void digisynth_midi_mod_wheel(digisynth *self, unsigned int data1,
                              unsigned int data2);
void digisynth_midi_pitchbend(digisynth *self, unsigned int data1,
                              unsigned int data2);
void digisynth_midi_control(digisynth *self, unsigned int data1,
                            unsigned int data2);

void digisynth_set_multi_melody_mode(digisynth *self, bool melody_mode);
void digisynth_set_melody_loop_num(digisynth *self, int melody_num,
                                   int loop_num);
int digisynth_add_melody(digisynth *self);
void digisynth_dupe_melody(midi_event **from, midi_event **to);
void digisynth_switch_melody(digisynth *self, unsigned int melody_num);
void digisynth_reset_melody(digisynth *self, unsigned int melody_num);
void digisynth_reset_melody_all(digisynth *self);
void digisynth_reset_voices(digisynth *self);
void digisynth_melody_to_string(digisynth *self, int melody_num,
                                wchar_t scratch[33]);
int digisynth_add_event(digisynth *self, int pattern_num, midi_event *ev);
void digisynth_delete_event(digisynth *ms, int pat_num, int tick);

midi_event **digisynth_get_midi_loop(digisynth *self);
midi_event **digisynth_copy_midi_loop(digisynth *self, int pattern_num);
void digisynth_add_midi_loop(digisynth *self, midi_event **events,
                             int pattern_num);
void digisynth_replace_midi_loop(digisynth *ms, midi_event **events,
                                 int melody_num);
void digisynth_toggle_delay_mode(digisynth *ms);

void digisynth_rand_settings(digisynth *ms);
void digisynth_print_melodies(digisynth *ms);
void digisynth_print_settings(digisynth *ms);
bool digisynth_save_settings(digisynth *ms, char *preset_name);
bool digisynth_load_settings(digisynth *ms, char *preset_name);
bool digisynth_list_presets(void);
bool digisynth_check_if_preset_exists(char *preset_to_find);

void digisynth_nudge_melody(digisynth *ms, int melody_num, int sixteenth);
bool is_valid_melody_num(digisynth *ns, int melody_num);

void digisynth_handle_midi_note(digisynth *ms, int note, int velocity,
                                bool update_last_midi);
void digisynth_set_arpeggiate(digisynth *ms, bool b);
void digisynth_set_arpeggiate_latch(digisynth *ms, bool b);
void digisynth_set_arpeggiate_single_note_repeat(digisynth *ms, bool b);
void digisynth_set_arpeggiate_octave_range(digisynth *ms, int val);
void digisynth_set_arpeggiate_mode(digisynth *ms, unsigned int mode);
void digisynth_set_arpeggiate_rate(digisynth *ms, unsigned int mode);

void digisynth_import_midi_from_file(digisynth *ms, char *filename);
void digisynth_set_filter_mod(digisynth *ms, double mod);
void digisynth_del_self(digisynth *ms);

void digisynth_set_generate_mode(digisynth *ms, bool b);
void digisynth_set_morph_mode(digisynth *ms, bool b);
void digisynth_set_backup_mode(digisynth *ms, bool b);
void digisynth_morph(digisynth *ms);
int digisynth_get_notes_from_melody(midi_event **melody,
                                    int return_midi_notes[10]);

void digisynth_sg_start(void *self);
void digisynth_sg_stop(void *self);

int digisynth_get_num_tracks(void *self);
int digisynth_get_num_notes(digisynth *ms);
void digisynth_make_active_track(void *self, int pattern_num);

void digisynth_print(digisynth *ms);
void digisynth_add_note(digisynth *ms, int pattern_num, int step,
                        int midi_note);
void digisynth_rm_note(digisynth *ms, int pattern_num, int step);
void digisynth_mv_note(digisynth *ms, int pattern_num, int fromstep,
                       int tostep);
void digisynth_add_micro_note(digisynth *ms, int pattern_num, int step,
                              int midi_note);
void digisynth_rm_micro_note(digisynth *ms, int pattern_num, int step);
void digisynth_mv_micro_note(digisynth *ms, int pattern_num, int fromstep,
                             int tostep);

void digisynth_set_attack_time_ms(digisynth *ms, double val);
void digisynth_set_decay_time_ms(digisynth *ms, double val);
void digisynth_set_release_time_ms(digisynth *ms, double val);
void digisynth_set_delay_feedback_pct(digisynth *ms, double val);
void digisynth_set_delay_ratio(digisynth *ms, double val);
void digisynth_set_delay_mode(digisynth *ms, unsigned int val);
void digisynth_set_delay_time_ms(digisynth *ms, double val);
void digisynth_set_delay_wetmix(digisynth *ms, double val);
void digisynth_set_detune(digisynth *ms, double val);
void digisynth_set_eg1_dca_int(digisynth *ms, double val);
void digisynth_set_eg1_filter_int(digisynth *ms, double val);
void digisynth_set_eg1_osc_int(digisynth *ms, double val);
void digisynth_set_filter_fc(digisynth *ms, double val);
void digisynth_set_filter_fq(digisynth *ms, double val);
void digisynth_set_filter_type(digisynth *ms, unsigned int val);
void digisynth_set_filter_saturation(digisynth *ms, double val);
void digisynth_set_filter_nlp(digisynth *ms, unsigned int val);
void digisynth_set_keytrack_int(digisynth *ms, double val);
void digisynth_set_keytrack(digisynth *ms, unsigned int val);
void digisynth_set_legato_mode(digisynth *ms, unsigned int val);
// LFO
void digisynth_set_lfo_amp_int(digisynth *ms, int lfo_num, double val);
void digisynth_set_lfo_amp(digisynth *ms, int lfo_num, double val);
void digisynth_set_lfo_filter_fc_int(digisynth *ms, int lfo_num, double val);
void digisynth_set_lfo_rate(digisynth *ms, int lfo_num, double val);
void digisynth_set_lfo_pan_int(digisynth *ms, int lfo_num, double val);
void digisynth_set_lfo_pitch(digisynth *ms, int lfo_num, double val);
void digisynth_set_lfo_wave(digisynth *ms, int lfo_num, unsigned int val);

void digisynth_set_note_to_decay_scaling(digisynth *ms, unsigned int val);
void digisynth_set_noise_osc_db(digisynth *ms, double val);
void digisynth_set_octave(digisynth *ms, int val);
void digisynth_set_pitchbend_range(digisynth *ms, int val);
void digisynth_set_portamento_time_ms(digisynth *ms, double val);
void digisynth_set_pulsewidth_pct(digisynth *ms, double val);
void digisynth_set_sub_osc_db(digisynth *ms, double val);
void digisynth_set_sustain(digisynth *ms, double val);
void digisynth_set_sustain_time_ms(digisynth *ms, double val);
void digisynth_set_sustain_time_sixteenth(digisynth *ms, double val);
void digisynth_set_sustain_override(digisynth *ms, bool b);
void digisynth_set_velocity_to_attack_scaling(digisynth *ms, unsigned int val);
void digisynth_set_voice_mode(digisynth *ms, unsigned int val);
void digisynth_set_vol(digisynth *ms, double val);
void digisynth_set_reset_to_zero(digisynth *ms, unsigned int val);
