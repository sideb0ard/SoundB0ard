#ifndef SBSHELL_VOICE_H_
#define SBSHELL_VOICE_H_

#include <dca.h>
#include <envelope_generator.h>
#include <filter.h>
#include <lfo.h>
#include <modmatrix.h>
#include <oscillator.h>
#include <synthfunctions.h>

class Voice
{
  public:
    Voice();
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

    bool m_note_on;
    bool hard_sync;
    int m_timestamp;

    int m_midi_note_number;
    int m_midi_note_number_pending;
    int m_midi_velocity;
    int m_midi_velocity_pending;

    /////////////////////////////
    Oscillator *m_osc1;
    Oscillator *m_osc2;
    Oscillator *m_osc3;
    Oscillator *m_osc4;

    Filter *m_filter1;
    Filter *m_filter2;

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

    unsigned int m_voice_mode;
    double m_hs_ratio; // hard sync

    unsigned int m_legato_mode;

    // pitch-bending for note-steal operation
    double m_osc_pitch;
    double m_osc_pitch_pending;

    double m_portamento_time_msec;
    double m_portamento_start;

    double m_modulo_portamento;
    double m_portamento_inc;

    double m_portamento_semitones;

    bool m_note_pending;

    double m_default_mod_intensity;
    double m_default_mod_range;
};

#endif // SBSHELL_VOICE_H_
