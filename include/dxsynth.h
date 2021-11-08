#pragma once

#include <dca.h>
#include <dxsynth_voice.h>
#include <envelope_generator.h>
#include <filter.h>
#include <midimaaan.h>
#include <modmatrix.h>
#include <oscillator.h>
#include <soundgenerator.h>
#include <voice.h>

#define MAX_DX_VOICES 16

enum { DX1, DX2, DX3, DX4, DX5, DX6, DX7, DX8, MAXDX };

static const char DX_PRESET_FILENAME[] = "settings/dxpresets.dat";

typedef struct dxsynthsettings {
  char m_settings_name[256];

  // LFO1     // lo/hi/def
  double m_lfo1_intensity;  // 0/1/0
  double m_lfo1_rate;       // 0.02 / 20 / 0.5
  unsigned int m_lfo1_waveform;
  unsigned int m_lfo1_mod_dest1;  // none, AmpMod, Vibrato
  unsigned int m_lfo1_mod_dest2;
  unsigned int m_lfo1_mod_dest3;
  unsigned int m_lfo1_mod_dest4;

  // OP1
  unsigned int m_op1_waveform;  // SINE, SAW, TRI, SQ
  double m_op1_ratio;           // 0.1/10/1
  double m_op1_detune_cents;    // -100/100/0
  double m_eg1_attack_ms;       // 0/5000/100
  double m_eg1_decay_ms;
  double m_eg1_sustain_lvl;  // 0/1/0.707
  double m_eg1_release_ms;
  double m_op1_output_lvl;  // 0/99/75

  // OP2
  unsigned int m_op2_waveform;  // SINE, SAW, TRI, SQ
  double m_op2_ratio;
  double m_op2_detune_cents;
  double m_eg2_attack_ms;
  double m_eg2_decay_ms;
  double m_eg2_sustain_lvl;
  double m_eg2_release_ms;
  double m_op2_output_lvl;

  // OP3
  unsigned int m_op3_waveform;  // SINE, SAW, TRI, SQ
  double m_op3_ratio;
  double m_op3_detune_cents;
  double m_eg3_attack_ms;
  double m_eg3_decay_ms;
  double m_eg3_sustain_lvl;
  double m_eg3_release_ms;
  double m_op3_output_lvl;

  // OP4
  unsigned int m_op4_waveform;  // SINE, SAW, TRI, SQ
  double m_op4_ratio;
  double m_op4_detune_cents;
  double m_eg4_attack_ms;
  double m_eg4_decay_ms;
  double m_eg4_sustain_lvl;
  double m_eg4_release_ms;
  double m_op4_output_lvl;
  double m_op4_feedback;  // 0/70/0

  // VOICE
  double m_portamento_time_ms;  // 0/5000/0
  double m_volume_db;           // -96/20/0
  int m_pitchbend_range;        // 0/12/1
  unsigned int m_voice_mode;    // DX[1-8];
  bool m_velocity_to_attack_scaling;
  bool m_note_number_to_decay_scaling;
  bool m_reset_to_zero;
  bool m_legato_mode;

} dxsynthsettings;

class DXSynth : public SoundGenerator {
 public:
  DXSynth();
  ~DXSynth() = default;
  stereo_val genNext(mixer_timing_info tinfo) override;
  std::string Info() override;
  std::string Status() override;
  void start() override;
  void stop() override;
  void noteOn(midi_event ev) override;
  void noteOff(midi_event ev) override;
  void allNotesOff() override;
  void ChordOn(midi_event ev) override;
  void control(midi_event ev) override;
  void pitchBend(midi_event ev) override;
  void randomize() override;
  void SetParam(std::string name, double val) override;
  double GetParam(std::string name) override;
  void Load(std::string preset_name) override;
  void Save(std::string preset_name) override;
  void ListPresets() override;

 public:
  std::array<std::shared_ptr<DXSynthVoice>, MAX_VOICES> voices_;

  // global modmatrix, core is shared by all voices
  ModulationMatrix modmatrix;  // routing structure for sound generation

  GlobalSynthParams global_synth_params;

  dxsynthsettings m_settings;
  dxsynthsettings m_settings_backup_while_getting_crazy;

  int active_midi_osc;  // for midi controller routing

  double m_last_note_frequency;

  void Reset();
  void Update();

  bool PrepareForPlay();

  void IncrementVoiceTimestamps();
  std::shared_ptr<DXSynthVoice> GetOldestVoice();
  std::shared_ptr<DXSynthVoice> GetOldestVoiceWithNote(int midi_note);

  void ResetVoices();

  bool CheckIfPresetExists(char *preset_to_find);

  void SetBitwise(bool b);
  void SetBitwiseMode(int mode);

  void SetFilterMod(double mod);

  void SetLFO1Intensity(double val);
  void SetLFO1Rate(double val);
  void SetLFO1Waveform(unsigned int val);
  void SetLFO1ModDest(unsigned int moddest, unsigned int dest);

  void SetOpWaveform(unsigned int op, unsigned int val);
  void SetOpRatio(unsigned int op, double val);
  void SetOpDetune(unsigned int op, double val);
  void SetEGAttackMs(unsigned int eg, double val);
  void SetEGDecayMs(unsigned int eg, double val);
  void SetEGReleaseMs(unsigned int eg, double val);
  void SetEGSustainLevel(unsigned int eg, double val);
  void SetOpOutputLevel(unsigned int op, double val);
  void SetOp4Feedback(double val);

  void SetPortamentoTimeMs(double val);
  void SetVolumeDb(double val);
  void SetPitchbendRange(unsigned int val);
  void SetVoiceMode(unsigned int val);
  void SetVelocityToAttackScaling(bool b);
  void SetNoteNumberToDecayScaling(bool b);
  void SetResetToZero(bool b);
  void SetLegatoMode(bool b);
  void SetActiveMidiOsc(int oscnum);
};
