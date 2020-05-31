#pragma once

#include <dca.h>
#include <dxsynth_voice.h>
#include <envelope_generator.h>
#include <filter.h>
#include <keys.h>
#include <midimaaan.h>
#include <modmatrix.h>
#include <oscillator.h>
#include <soundgenerator.h>

#define MAX_DX_VOICES 16

enum
{
    DX1,
    DX2,
    DX3,
    DX4,
    DX5,
    DX6,
    DX7,
    DX8,
    MAXDX
};

static const char DX_PRESET_FILENAME[] = "settings/dxpresets.dat";

typedef struct dxsynthsettings
{
    char m_settings_name[256];

    // LFO1     // lfo/hi/def
    double m_lfo1_intensity; // 0/1/0
    double m_lfo1_rate;      // 0.02 / 20 / 0.5
    unsigned int m_lfo1_waveform;
    unsigned int m_lfo1_mod_dest1; // none, AmpMod, Vibrato
    unsigned int m_lfo1_mod_dest2;
    unsigned int m_lfo1_mod_dest3;
    unsigned int m_lfo1_mod_dest4;

    // OP1
    unsigned int m_op1_waveform; // SINE, SAW, TRI, SQ
    double m_op1_ratio;          // 0.1/10/1
    double m_op1_detune_cents;   // -100/100/0
    double m_eg1_attack_ms;      // 0/5000/100
    double m_eg1_decay_ms;
    double m_eg1_sustain_lvl; // 0/1/0.707
    double m_eg1_release_ms;
    double m_op1_output_lvl; // 0/99/75

    // OP2
    unsigned int m_op2_waveform; // SINE, SAW, TRI, SQ
    double m_op2_ratio;
    double m_op2_detune_cents;
    double m_eg2_attack_ms;
    double m_eg2_decay_ms;
    double m_eg2_sustain_lvl;
    double m_eg2_release_ms;
    double m_op2_output_lvl;

    // OP3
    unsigned int m_op3_waveform; // SINE, SAW, TRI, SQ
    double m_op3_ratio;
    double m_op3_detune_cents;
    double m_eg3_attack_ms;
    double m_eg3_decay_ms;
    double m_eg3_sustain_lvl;
    double m_eg3_release_ms;
    double m_op3_output_lvl;

    // OP4
    unsigned int m_op4_waveform; // SINE, SAW, TRI, SQ
    double m_op4_ratio;
    double m_op4_detune_cents;
    double m_eg4_attack_ms;
    double m_eg4_decay_ms;
    double m_eg4_sustain_lvl;
    double m_eg4_release_ms;
    double m_op4_output_lvl;
    double m_op4_feedback; // 0/70/0

    // VOICE
    double m_portamento_time_ms; // 0/5000/0
    double m_volume_db;          // -96/20/0
    int m_pitchbend_range;       // 0/12/1
    unsigned int m_voice_mode;   // DX[1-8];
    bool m_velocity_to_attack_scaling;
    bool m_note_number_to_decay_scaling;
    bool m_reset_to_zero;
    bool m_legato_mode;

} dxsynthsettings;

class dxsynth : public SoundGenerator
{
  public:
    dxsynth();
    ~dxsynth();
    stereo_val genNext() override;
    std::string Info() override;
    std::string Status() override;
    void start() override;
    void stop() override;
    void noteOn(midi_event ev) override;
    void noteOff(midi_event ev) override;
    void allNotesOff() override;
    void control(midi_event ev) override;
    void pitchBend(midi_event ev) override;
    void randomize() override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;
    void Load(std::string preset_name) override;
    void Save(std::string preset_name) override;

  public:
    dxsynth_voice *m_voices[MAX_DX_VOICES];

    // global modmatrix, core is shared by all voices
    modmatrix m_global_modmatrix; // routing structure for sound generation
    global_synth_params m_global_synth_params;

    dxsynthsettings m_settings;
    dxsynthsettings m_settings_backup_while_getting_crazy;

    int active_midi_osc; // for midi controller routing

    double m_last_note_frequency;
};

void dxsynth_reset(dxsynth *dx);

bool dxsynth_prepare_for_play(dxsynth *synth);
void dxsynth_update(dxsynth *synth);

void dxsynth_increment_voice_timestamps(dxsynth *synth);
dxsynth_voice *dxsynth_get_oldest_voice(dxsynth *synth);
dxsynth_voice *dxsynth_get_oldest_voice_with_note(dxsynth *synth,
                                                  int midi_note);

void dxsynth_reset_voices(dxsynth *self);

void dxsynth_print_settings(dxsynth *ms);
void dxsynth_print_modulation_routings(dxsynth *ms);
void dxsynth_print_lfo1_routing_info(dxsynth *ms, wchar_t *scratch);
void dxsynth_print_lfo2_routing_info(dxsynth *ms, wchar_t *scratch);
void dxsynth_print_eg1_routing_info(dxsynth *ms, wchar_t *scratch);
void dxsynth_print_eg2_routing_info(dxsynth *ms, wchar_t *scratch);

bool dxsynth_list_presets(void);
bool dxsynth_check_if_preset_exists(char *preset_to_find);

// void dxsynth_set_arpeggiate(dxsynth *ms, bool b);
// void dxsynth_set_arpeggiate_latch(dxsynth *ms, bool b);
// void dxsynth_set_arpeggiate_single_note_repeat(dxsynth *ms, bool b);
// void dxsynth_set_arpeggiate_octave_range(dxsynth *ms, int val);
// void dxsynth_set_arpeggiate_mode(dxsynth *ms, unsigned int mode);
// void dxsynth_set_arpeggiate_rate(dxsynth *ms, unsigned int mode);

void dxsynth_set_bitwise(dxsynth *ms, bool b);
void dxsynth_set_bitwise_mode(dxsynth *ms, int mode);

void dxsynth_set_filter_mod(dxsynth *ms, double mod);

void dxsynth_print(dxsynth *ms);

void dxsynth_set_lfo1_intensity(dxsynth *d, double val);
void dxsynth_set_lfo1_rate(dxsynth *d, double val);
void dxsynth_set_lfo1_waveform(dxsynth *d, unsigned int val);
void dxsynth_set_lfo1_mod_dest(dxsynth *d, unsigned int mod_dest,
                               unsigned int dest);

void dxsynth_set_op_waveform(dxsynth *d, unsigned int op, unsigned int val);
void dxsynth_set_op_ratio(dxsynth *d, unsigned int op, double val);
void dxsynth_set_op_detune(dxsynth *d, unsigned int op, double val);
void dxsynth_set_eg_attack_ms(dxsynth *d, unsigned int eg, double val);
void dxsynth_set_eg_decay_ms(dxsynth *d, unsigned int eg, double val);
void dxsynth_set_eg_release_ms(dxsynth *d, unsigned int eg, double val);
void dxsynth_set_eg_sustain_lvl(dxsynth *d, unsigned int eg, double val);
void dxsynth_set_op_output_lvl(dxsynth *d, unsigned int op, double val);
void dxsynth_set_op4_feedback(dxsynth *d, double val);

void dxsynth_set_portamento_time_ms(dxsynth *d, double val);
void dxsynth_set_volume_db(dxsynth *d, double val);
void dxsynth_set_pitchbend_range(dxsynth *d, unsigned int val);
void dxsynth_set_voice_mode(dxsynth *d, unsigned int val);
void dxsynth_set_velocity_to_attack_scaling(dxsynth *d, bool b);
void dxsynth_set_note_number_to_decay_scaling(dxsynth *d, bool b);
void dxsynth_set_reset_to_zero(dxsynth *d, bool b);
void dxsynth_set_legato_mode(dxsynth *d, bool b);
void dxsynth_set_active_midi_osc(dxsynth *dx, int osc_num);
