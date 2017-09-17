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
#include "synthbase.h"

static const char PRESET_FILENAME[] = "settings/synthpresets.dat";

typedef struct synthsettings
{
    char m_settings_name[256];

    unsigned int m_voice_mode;

    unsigned int m_lfo1_waveform;
    unsigned int m_lfo1_dest;
    double m_lfo1_rate;
    double m_lfo1_amplitude;
    double m_lfo1_amp_intensity;
    double m_lfo1_filter_fc_intensity;
    double m_lfo1_osc_pitch_intensity;
    double m_lfo1_pan_intensity;

    unsigned int m_lfo2_waveform;
    unsigned int m_lfo2_dest;
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

typedef struct minisynth
{
    SOUNDGEN sound_generator;
    synthbase base;

    bool active;

    minisynth_voice *m_voices[MAX_VOICES];

    // global modmatrix, core is shared by all voices
    modmatrix m_ms_modmatrix; // routing structure for sound generation
    global_synth_params m_global_synth_params;

    double m_last_note_frequency;

    unsigned int m_midi_rx_channel;

    stereodelay m_delay_fx;

    synthsettings m_settings;
    synthsettings m_settings_backup_while_getting_crazy;

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
void minisynth_sg_start(void *self);
void minisynth_sg_stop(void *self);

////////////////////////////////////

bool minisynth_prepare_for_play(minisynth *synth);
void minisynth_stop(minisynth *ms);
void minisynth_update(minisynth *synth);

void minisynth_midi_control(minisynth *self, unsigned int data1,
                            unsigned int data2);

void minisynth_increment_voice_timestamps(minisynth *synth);
minisynth_voice *minisynth_get_oldest_voice(minisynth *synth);
minisynth_voice *minisynth_get_oldest_voice_with_note(minisynth *synth,
                                                      unsigned int midi_note);

void minisynth_handle_midi_note(minisynth *ms, int note, int velocity,
                                bool update_last_midi);
bool minisynth_midi_note_on(minisynth *self, unsigned int midinote,
                            unsigned int velocity);
bool minisynth_midi_note_off(minisynth *self, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off);
void minisynth_midi_mod_wheel(minisynth *self, unsigned int data1,
                              unsigned int data2);
void minisynth_midi_pitchbend(minisynth *self, unsigned int data1,
                              unsigned int data2);
void minisynth_reset_voices(minisynth *self);

void minisynth_toggle_delay_mode(minisynth *ms);

void minisynth_rand_settings(minisynth *ms);
void minisynth_print_settings(minisynth *ms);
bool minisynth_save_settings(minisynth *ms, char *preset_name);
bool minisynth_load_settings(minisynth *ms, char *preset_name);
bool minisynth_list_presets(void);
bool minisynth_check_if_preset_exists(char *preset_to_find);

void minisynth_set_arpeggiate(minisynth *ms, bool b);
void minisynth_set_arpeggiate_latch(minisynth *ms, bool b);
void minisynth_set_arpeggiate_single_note_repeat(minisynth *ms, bool b);
void minisynth_set_arpeggiate_octave_range(minisynth *ms, int val);
void minisynth_set_arpeggiate_mode(minisynth *ms, unsigned int mode);
void minisynth_set_arpeggiate_rate(minisynth *ms, unsigned int mode);

void minisynth_set_filter_mod(minisynth *ms, double mod);
void minisynth_del_self(minisynth *ms);

void minisynth_print(minisynth *ms);

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
