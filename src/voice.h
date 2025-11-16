#ifndef SBSHELL_VOICE_H_
#define SBSHELL_VOICE_H_

#include <dca.h>
#include <envelope_generator.h>
#include <filter.h>
#include <lfo.h>
#include <modmatrix.h>
#include <oscillator.h>
#include <synthfunctions.h>

class Voice {
 public:
  Voice() : m_global_voice_params(nullptr), m_global_synth_params(nullptr) {}
  virtual ~Voice() = default;

  void SetModMatrixCore(std::vector<std::shared_ptr<ModMatrixRow>> &matrix);
  virtual void InitializeModMatrix(ModulationMatrix *matrix);
  virtual void InitGlobalParameters(GlobalSynthParams *sp);
  virtual bool IsActiveVoice();
  virtual bool CanNoteOff();
  virtual bool IsVoiceDone();
  virtual bool InLegatoMode();
  virtual void PrepareForPlay();
  virtual void Update();
  virtual void Reset();
  virtual bool DoVoice(double *left_output, double *right_output);

  void NoteOn(int midi_note, int midi_velocity, double frequency,
              double last_note_frequency);
  void NoteOff(int midi_note);
  void SetSustainOverride(bool b);

 public:
  // shared by source and dest
  ModulationMatrix modmatrix;

  bool m_note_on{false};
  bool hard_sync{false};
  int m_timestamp{0};

  int m_midi_note_number{0};
  int m_midi_note_number_pending{0};
  int m_midi_velocity{0};
  int m_midi_velocity_pending{0};

  /////////////////////////////
  Oscillator *m_osc1{nullptr};
  Oscillator *m_osc2{nullptr};
  Oscillator *m_osc3{nullptr};
  Oscillator *m_osc4{nullptr};

  Filter *m_filter1{nullptr};
  Filter *m_filter2{nullptr};

  EnvelopeGenerator m_eg1;
  EnvelopeGenerator m_eg2;
  EnvelopeGenerator m_eg3;
  EnvelopeGenerator m_eg4;

  LFO m_lfo1;
  LFO m_lfo2;

  DCA m_dca;
  /////////////////////////////

  GlobalVoiceParams *m_global_voice_params;
  GlobalSynthParams *m_global_synth_params;

  unsigned int m_voice_mode{0};
  double m_hs_ratio{1};  // hard sync

  unsigned int m_legato_mode{LEGATO};

  // pitch-bending for note-steal operation
  double m_osc_pitch{OSC_FO_DEFAULT};
  double m_osc_pitch_pending{OSC_FO_DEFAULT};

  double m_portamento_time_msec{0};
  double m_portamento_start{OSC_FO_DEFAULT};

  double m_modulo_portamento{0};
  double m_portamento_inc{0};

  double m_portamento_semitones{0};

  bool m_note_pending{false};

  double m_default_mod_intensity{1};
  double m_default_mod_range{1};
};

#endif  // SBSHELL_VOICE_H_
