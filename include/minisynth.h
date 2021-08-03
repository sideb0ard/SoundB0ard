#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <array>

#include <dca.h>
#include <envelope_generator.h>
#include <filter.h>
#include <keys.h>
#include <midimaaan.h>
#include <minisynth_voice.h>
#include <modmatrix.h>
#include <oscillator.h>
#include <soundgenerator.h>

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

class MiniSynth : public SoundGenerator
{
  public:
    MiniSynth();
    ~MiniSynth() = default;

    stereo_val genNext() override;
    std::string Info() override;
    std::string Status() override;
    void start() override;
    void stop() override;
    void noteOn(midi_event ev) override;
    void noteOff(midi_event ev) override;
    void ChordOn(midi_event ev) override;
    void allNotesOff() override;
    void control(midi_event ev) override;
    void pitchBend(midi_event ev) override;
    void randomize() override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;
    void Save(std::string preset_name) override;
    void Load(std::string preset_name) override;

    void LoadDefaults();

    std::array<std::shared_ptr<MiniSynthVoice>, MAX_VOICES> voices_;

    // global modmatrix, core is shared by all voices
    ModulationMatrix modmatrix; // routing structure for sound generation
    GlobalSynthParams m_global_synth_params;

    double m_last_note_frequency;
    unsigned int m_midi_rx_channel;

    synthsettings m_settings;
    synthsettings m_settings_backup_while_getting_crazy;

    bool PrepareForPlay();
    void Stop();
    void Update();

    void MidiControl(unsigned int data1, unsigned int data2);

    void IncrementVoiceTimestamps();
    std::shared_ptr<MiniSynthVoice> GetOldestVoice();
    std::shared_ptr<MiniSynthVoice> GetOldestVoiceWithNote(int midi_note);

    void ResetVoices();

    bool CheckIfPresetExists(char *preset_to_find);

    void SetArpeggiate(bool b);
    void SetArpeggiateLatch(bool b);
    void SetArpeggiateSingleNoteRepeat(bool b);
    void SetArpeggiateOctaveRange(int val);
    void SetArpeggiateMode(unsigned int mode);
    void SetArpeggiateRate(unsigned int mode);

    void SetGenerate(bool b);
    void SetGenerateSrc(int src);

    void SetFilterMod(double mod);

    void SetDetune(double val);

    void SetEgAttackTimeMs(unsigned int osc_num, double val);
    void SetEgDecayTimeMs(unsigned int osc_num, double val);
    void SetEgReleaseTimeMs(unsigned int osc_num, double val);
    void SetEgDcaInt(unsigned int osc_num, double val);
    void SetEgDcaEnable(unsigned int osc_num, int val);
    void SetEgFilterInt(unsigned int osc_num, double val);
    void SetEgFilterEnable(unsigned int osc_num, int val);
    void SetEgOscInt(unsigned int osc_num, double val);
    void SetEgOscEnable(unsigned int osc_num, int val);
    void SetEgSustain(unsigned int osc_num, double val);
    void SetEgSustainOverride(unsigned int osc_num, bool b);

    void SetFilterFc(double val);
    void SetFilterFq(double val);
    void SetFilterType(unsigned int val);
    void SetFilterSaturation(double val);
    void SetFilterNlp(unsigned int val);
    void SetKeytrackInt(double val);
    void SetKeytrack(unsigned int val);
    void SetLegatoMode(unsigned int val);
    // OSC

    // LFO
    void SetLFOAmpEnable(int lfo_num, int val);
    void SetLFOAmpInt(int lfo_num, double val);
    void SetLFOAmp(int lfo_num, double val);
    void SetLFOFilterEnable(int lfo_num, int val);
    void SetLFOFilterFcInt(int lfo_num, double val);
    void SetLFORate(int lfo_num, double val);
    void SetLFOPanEnable(int lfo_num, int val);
    void SetLFOPanInt(int lfo_num, double val);
    void SetLFOOscEnable(int lfo_num, int val);
    void SetLFOOscInt(int lfo_num, double val);
    void SetLFOWave(int lfo_num, unsigned int val);
    void SetLFOMode(int lfo_num, unsigned int val);
    void SetLFOPulsewidthEnable(int lfo_num, unsigned int val);
    void SetLFOPulsewidthInt(int lfo_num, double val);

    void SetNoteToDecayScaling(unsigned int val);
    void SetNoiseOscDb(double val);
    void SetOctave(int val);
    void SetOscType(int osc, unsigned int osc_type);
    void SetOscAmp(unsigned int osc_num, double val);
    void SetPitchbendRange(int val);
    void SetPortamentoTimeMs(double val);
    void SetPulsewidthPct(double val);
    void SetSubOscDb(double val);
    void SetVelocityToAttackScaling(unsigned int val);
    void SetVoiceMode(unsigned int val);
    void SetResetToZero(unsigned int val);
    void SetMonophonic(bool b);
    void SetHardSync(bool b);
};
