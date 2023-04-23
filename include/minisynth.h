#pragma once

#include <dca.h>
#include <defjams.h>
#include <envelope_generator.h>
#include <filter.h>
#include <midimaaan.h>
#include <minisynth_voice.h>
#include <modmatrix.h>
#include <oscillator.h>
#include <soundgenerator.h>
#include <stdbool.h>

#include <array>

namespace SBAudio {

typedef struct synthsettings {
  char m_settings_name[256];

  unsigned int m_voice_mode{0};
  bool m_monophonic{false};
  bool hard_sync{false};

  unsigned int osc1_wave{0};
  double osc1_amp{01};
  int osc1_oct{0};
  int osc1_semis{0};
  int osc1_cents{0};

  unsigned int osc2_wave{0};
  double osc2_amp{0};
  int osc2_oct{0};
  int osc2_semis{0};
  int osc2_cents{0};

  unsigned int osc3_wave{0};
  double osc3_amp{0};
  int osc3_oct{0};
  int osc3_semis{0};
  int osc3_cents{0};

  unsigned int osc4_wave{0};
  double osc4_amp{0};
  int osc4_oct{0};
  int osc4_semis{0};
  int osc4_cents{0};

  unsigned int m_lfo1_waveform{0};
  unsigned int m_lfo1_dest{0};
  unsigned int m_lfo1_mode{0};
  double m_lfo1_rate{0};
  double m_lfo1_amplitude{0};

  // LFO1 -> OSC FO
  double m_lfo1_osc_pitch_intensity{0};
  bool m_lfo1_osc_pitch_enabled{0};

  // LFO1 -> FILTER CUTOFF
  double m_lfo1_filter_fc_intensity{0};
  bool m_lfo1_filter_fc_enabled{0};

  // LFO1 -> FILTER Q
  double m_lfo1_filter_q_intensity{0};
  bool m_lfo1_filter_q_enabled{0};

  // LFO1 -> DCA
  double m_lfo1_amp_intensity{0};
  bool m_lfo1_amp_enabled{0};
  double m_lfo1_pan_intensity{0};
  bool m_lfo1_pan_enabled{0};

  // LFO1 -> Pulse Width
  double m_lfo1_pulsewidth_intensity{0};
  bool m_lfo1_pulsewidth_enabled{0};

  unsigned int m_lfo2_waveform{0};
  unsigned int m_lfo2_dest{0};
  unsigned int m_lfo2_mode{0};
  double m_lfo2_rate{0};
  double m_lfo2_amplitude{0};

  // LFO2 -> OSC FO
  double m_lfo2_osc_pitch_intensity{0};
  bool m_lfo2_osc_pitch_enabled{0};

  // LFO2 -> FILTER CUTOFF
  double m_lfo2_filter_fc_intensity{0};
  bool m_lfo2_filter_fc_enabled{0};

  // LFO1 -> FILTER Q
  double m_lfo2_filter_q_intensity{0};
  bool m_lfo2_filter_q_enabled{0};

  // LFO2 -> DCA
  double m_lfo2_amp_intensity{0};
  bool m_lfo2_amp_enabled{0};
  double m_lfo2_pan_intensity{0};
  bool m_lfo2_pan_enabled{0};

  // LFO2 -> Pulse Width
  double m_lfo2_pulsewidth_intensity{0};
  bool m_lfo2_pulsewidth_enabled{0};

  // EG1  ////////////////////////////////////////
  double m_eg1_attack_time_msec{0};
  double m_eg1_decay_time_msec{0};
  double m_eg1_release_time_msec{0};
  double m_eg1_sustain_level{0};

  // EG1 -> OSC
  double m_eg1_osc_intensity{0};
  bool m_eg1_osc_enabled{0};

  // EG1 -> FILTER
  double m_eg1_filter_intensity{0};
  bool m_eg1_filter_enabled{0};

  // EG1 -> DCA
  double m_eg1_dca_intensity{0};
  bool m_eg1_dca_enabled{0};

  unsigned int m_eg1_sustain_override{0};

  // EG2  ////////////////////////////////////////
  double m_eg2_attack_time_msec{0};
  double m_eg2_decay_time_msec{0};
  double m_eg2_release_time_msec{0};
  double m_eg2_sustain_level{0};

  // EG2 -> OSC
  double m_eg2_osc_intensity{0};
  bool m_eg2_osc_enabled{0};

  // EG2 -> FILTER
  double m_eg2_filter_intensity{0};
  bool m_eg2_filter_enabled{0};

  // EG2 -> DCA
  double m_eg2_dca_intensity{0};
  bool m_eg2_dca_enabled{0};

  unsigned int m_eg2_sustain_override{0};

  ///////////////////////////////////////////////////////////////

  double m_filter_keytrack_intensity{0};

  int m_octave{0};
  int m_pitchbend_range{0};

  double m_fc_control{0};
  double m_q_control{0};

  double m_detune_cents{0};
  double m_pulse_width_pct{0};
  double m_sub_osc_db{0};
  double m_noise_osc_db{0};

  unsigned int m_legato_mode{0};
  unsigned int m_reset_to_zero{0};
  unsigned int m_filter_keytrack{0};
  unsigned int m_filter_type{0};
  double m_filter_saturation{0};

  unsigned int m_nlp{0};
  unsigned int m_velocity_to_attack_scaling{0};
  unsigned int m_note_number_to_decay_scaling{0};
  double m_portamento_time_msec{0};

  bool m_generate_active{0};
  unsigned m_generate_src{0};
} synthsettings;

class MiniSynth : public SoundGenerator {
 public:
  MiniSynth();
  ~MiniSynth() = default;

  StereoVal GenNext(mixer_timing_info tinfo) override;
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
  void Save(std::string preset_name) override;
  void ListPresets() override;
  void LoadPreset(std::string preset_name,
                  std::map<std::string, double> preset) override;

  void LoadDefaults();

  std::array<std::shared_ptr<MiniSynthVoice>, MAX_VOICES> voices_;

  // global modmatrix, core is shared by all voices
  ModulationMatrix modmatrix;  // routing structure for sound generation
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
  void SetOscAmp(unsigned int osc_num, double val);
  void SetOscSemitones(unsigned int osc_num, int val);
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

}  // namespace SBAudio
