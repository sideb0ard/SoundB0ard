#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "SoundGenerator.h"
#include "dca.h"
#include "envelope_generator.h"
#include "filter.h"
#include "keys.h"
#include "midimaaan.h"
#include "modmatrix.h"
#include "oscillator.h"

#include "minisynth_voice.h"

static const char MOOG_PRESET_FILENAME[] = "settings/moogpresets.dat";

typedef struct synthsettings
{
    char m_settings_name[256];

    unsigned int m_voice_mode;
    bool m_monophonic;
    bool hard_sync;

    unsigned int osc1_wave;
    double osc1_amp;
    int osc1_oct;
    int osc1_semis;
    int osc1_cents;

    unsigned int osc2_wave;
    double osc2_amp;
    int osc2_oct;
    int osc2_semis;
    int osc2_cents;

    unsigned int osc3_wave;
    double osc3_amp;
    int osc3_oct;
    int osc3_semis;
    int osc3_cents;

    unsigned int osc4_wave;
    double osc4_amp;
    int osc4_oct;
    int osc4_semis;
    int osc4_cents;

    unsigned int m_lfo1_waveform;
    unsigned int m_lfo1_dest;
    unsigned int m_lfo1_mode;
    double m_lfo1_rate;
    double m_lfo1_amplitude;

    // LFO1 -> OSC FO
    double m_lfo1_osc_pitch_intensity;
    bool m_lfo1_osc_pitch_enabled;

    // LFO1 -> FILTER
    double m_lfo1_filter_fc_intensity;
    bool m_lfo1_filter_fc_enabled;

    // LFO1 -> DCA
    double m_lfo1_amp_intensity;
    bool m_lfo1_amp_enabled;
    double m_lfo1_pan_intensity;
    bool m_lfo1_pan_enabled;

    // LFO1 -> Pulse Width
    double m_lfo1_pulsewidth_intensity;
    bool m_lfo1_pulsewidth_enabled;

    unsigned int m_lfo2_waveform;
    unsigned int m_lfo2_dest;
    unsigned int m_lfo2_mode;
    double m_lfo2_rate;
    double m_lfo2_amplitude;

    // LFO2 -> OSC FO
    double m_lfo2_osc_pitch_intensity;
    bool m_lfo2_osc_pitch_enabled;

    // LFO2 -> FILTER
    double m_lfo2_filter_fc_intensity;
    bool m_lfo2_filter_fc_enabled;

    // LFO2 -> DCA
    double m_lfo2_amp_intensity;
    bool m_lfo2_amp_enabled;
    double m_lfo2_pan_intensity;
    bool m_lfo2_pan_enabled;

    // LFO2 -> Pulse Width
    double m_lfo2_pulsewidth_intensity;
    bool m_lfo2_pulsewidth_enabled;

    // EG1  ////////////////////////////////////////
    double m_eg1_attack_time_msec;
    double m_eg1_decay_time_msec;
    double m_eg1_release_time_msec;
    double m_eg1_sustain_level;

    // EG1 -> OSC
    double m_eg1_osc_intensity;
    bool m_eg1_osc_enabled;

    // EG1 -> FILTER
    double m_eg1_filter_intensity;
    bool m_eg1_filter_enabled;

    // EG1 -> DCA
    double m_eg1_dca_intensity;
    bool m_eg1_dca_enabled;

    unsigned int m_eg1_sustain_override;

    // EG2  ////////////////////////////////////////
    double m_eg2_attack_time_msec;
    double m_eg2_decay_time_msec;
    double m_eg2_release_time_msec;
    double m_eg2_sustain_level;

    // EG2 -> OSC
    double m_eg2_osc_intensity;
    bool m_eg2_osc_enabled;

    // EG2 -> FILTER
    double m_eg2_filter_intensity;
    bool m_eg2_filter_enabled;

    // EG2 -> DCA
    double m_eg2_dca_intensity;
    bool m_eg2_dca_enabled;

    unsigned int m_eg2_sustain_override;

    ///////////////////////////////////////////////////////////////

    double m_filter_keytrack_intensity;

    int m_octave;
    int m_pitchbend_range;

    double m_fc_control;
    double m_q_control;

    double m_detune_cents;
    double m_pulse_width_pct;
    double m_sub_osc_db;
    double m_noise_osc_db;

    unsigned int m_legato_mode;
    unsigned int m_reset_to_zero;
    unsigned int m_filter_keytrack;
    unsigned int m_filter_type;
    double m_filter_saturation;

    unsigned int m_nlp;
    unsigned int m_velocity_to_attack_scaling;
    unsigned int m_note_number_to_decay_scaling;
    double m_portamento_time_msec;

    bool m_generate_active;
    unsigned m_generate_src;
} synthsettings;

class minisynth : public SoundGenerator
{
  public:
    minisynth();
    ~minisynth();
    stereo_val genNext() override;
    void status(wchar_t *status_string) override;
    void start() override;
    void stop() override;

    minisynth_voice *m_voices[MAX_VOICES];

    // global modmatrix, core is shared by all voices
    modmatrix m_ms_modmatrix; // routing structure for sound generation
    global_synth_params m_global_synth_params;

    double m_last_note_frequency;
    unsigned int m_midi_rx_channel;

    synthsettings m_settings;
    synthsettings m_settings_backup_while_getting_crazy;
};

void minisynth_load_defaults(minisynth *ms);

bool minisynth_prepare_for_play(minisynth *synth);
void minisynth_stop(minisynth *ms);
void minisynth_update(minisynth *synth);

void minisynth_midi_control(minisynth *self, unsigned int data1,
                            unsigned int data2);

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
void minisynth_reset_voices(minisynth *self);

void minisynth_rand_settings(minisynth *ms);

void minisynth_print_settings(minisynth *ms);
void minisynth_print_patterns(minisynth *ms);
void minisynth_print_modulation_routings(minisynth *ms);
void minisynth_print_lfo1_routing_info(minisynth *ms, wchar_t *scratch);
void minisynth_print_lfo2_routing_info(minisynth *ms, wchar_t *scratch);
void minisynth_print_eg1_routing_info(minisynth *ms, wchar_t *scratch);
void minisynth_print_eg2_routing_info(minisynth *ms, wchar_t *scratch);

bool minisynth_save_settings(minisynth *ms, char *preset_name);
bool minisynth_load_settings(minisynth *ms, char *preset_name);
bool minisynth_check_if_preset_exists(char *preset_to_find);

void minisynth_set_arpeggiate(minisynth *ms, bool b);
void minisynth_set_arpeggiate_latch(minisynth *ms, bool b);
void minisynth_set_arpeggiate_single_note_repeat(minisynth *ms, bool b);
void minisynth_set_arpeggiate_octave_range(minisynth *ms, int val);
void minisynth_set_arpeggiate_mode(minisynth *ms, unsigned int mode);
void minisynth_set_arpeggiate_rate(minisynth *ms, unsigned int mode);

void minisynth_set_generate(minisynth *ms, bool b);
void minisynth_set_generate_src(minisynth *ms, int src);

void minisynth_set_filter_mod(minisynth *ms, double mod);

void minisynth_print(minisynth *ms);
void minisynth_set_detune(minisynth *ms, double val);

void minisynth_set_eg_attack_time_ms(minisynth *ms, unsigned int osc_num,
                                     double val);
void minisynth_set_eg_decay_time_ms(minisynth *ms, unsigned int osc_num,
                                    double val);
void minisynth_set_eg_release_time_ms(minisynth *ms, unsigned int osc_num,
                                      double val);
void minisynth_set_eg_dca_int(minisynth *ms, unsigned int osc_num, double val);
void minisynth_set_eg_dca_enable(minisynth *ms, unsigned int osc_num, int val);
void minisynth_set_eg_filter_int(minisynth *ms, unsigned int osc_num,
                                 double val);
void minisynth_set_eg_filter_enable(minisynth *ms, unsigned int osc_num,
                                    int val);
void minisynth_set_eg_osc_int(minisynth *ms, unsigned int osc_num, double val);
void minisynth_set_eg_osc_enable(minisynth *ms, unsigned int osc_num, int val);
void minisynth_set_eg_sustain(minisynth *ms, unsigned int osc_num, double val);
void minisynth_set_eg_sustain_override(minisynth *ms, unsigned int osc_num,
                                       bool b);

void minisynth_set_filter_fc(minisynth *ms, double val);
void minisynth_set_filter_fq(minisynth *ms, double val);
void minisynth_set_filter_type(minisynth *ms, unsigned int val);
void minisynth_set_filter_saturation(minisynth *ms, double val);
void minisynth_set_filter_nlp(minisynth *ms, unsigned int val);
void minisynth_set_keytrack_int(minisynth *ms, double val);
void minisynth_set_keytrack(minisynth *ms, unsigned int val);
void minisynth_set_legato_mode(minisynth *ms, unsigned int val);
// OSC

// LFO
void minisynth_set_lfo_amp_enable(minisynth *ms, int lfo_num, int val);
void minisynth_set_lfo_amp_int(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_amp(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_filter_enable(minisynth *ms, int lfo_num, int val);
void minisynth_set_lfo_filter_fc_int(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_rate(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_pan_enable(minisynth *ms, int lfo_num, int val);
void minisynth_set_lfo_pan_int(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_osc_enable(minisynth *ms, int lfo_num, int val);
void minisynth_set_lfo_osc_int(minisynth *ms, int lfo_num, double val);
void minisynth_set_lfo_wave(minisynth *ms, int lfo_num, unsigned int val);
void minisynth_set_lfo_mode(minisynth *ms, int lfo_num, unsigned int val);
void minisynth_set_lfo_pulsewidth_enable(minisynth *ms, int lfo_num,
                                         unsigned int val);
void minisynth_set_lfo_pulsewidth_int(minisynth *ms, int lfo_num, double val);

void minisynth_set_note_to_decay_scaling(minisynth *ms, unsigned int val);
void minisynth_set_noise_osc_db(minisynth *ms, double val);
void minisynth_set_octave(minisynth *ms, int val);
void minisynth_set_osc_type(minisynth *ms, int osc, unsigned int osc_type);
void minisynth_set_osc_cents(minisynth *ms, unsigned int osc_num, double val);
void minisynth_set_osc_amp(minisynth *ms, unsigned int osc_num, double val);
void minisynth_set_pitchbend_range(minisynth *ms, int val);
void minisynth_set_portamento_time_ms(minisynth *ms, double val);
void minisynth_set_pulsewidth_pct(minisynth *ms, double val);
void minisynth_set_sub_osc_db(minisynth *ms, double val);
void minisynth_set_velocity_to_attack_scaling(minisynth *ms, unsigned int val);
void minisynth_set_voice_mode(minisynth *ms, unsigned int val);
void minisynth_set_reset_to_zero(minisynth *ms, unsigned int val);
void minisynth_set_monophonic(minisynth *ms, bool b);
void minisynth_set_hard_sync(minisynth *ms, bool val);
