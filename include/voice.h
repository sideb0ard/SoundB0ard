#ifndef SBSHELL_VOICE_H_
#define SBSHELL_VOICE_H_

#include "dca.h"
#include "envelope_generator.h"
#include "filter.h"
#include "lfo.h"
#include "modmatrix.h"
#include "oscillator.h"
#include "synthfunctions.h"

typedef struct voice
{

    // shared by source and dest
    modmatrix m_v_modmatrix;

    bool m_note_on;
    bool hard_sync;
    int m_timestamp;

    int m_midi_note_number;
    int m_midi_note_number_pending;
    int m_midi_velocity;
    int m_midi_velocity_pending;

    /////////////////////////////
    oscillator *m_osc1;
    oscillator *m_osc2;
    oscillator *m_osc3;
    oscillator *m_osc4;

    filter *m_filter1;
    filter *m_filter2;

    envelope_generator m_eg1;
    envelope_generator m_eg2;
    envelope_generator m_eg3;
    envelope_generator m_eg4;

    lfo m_lfo1;
    lfo m_lfo2;

    dca m_dca;
    /////////////////////////////

    global_voice_params *m_global_voice_params;
    global_synth_params *m_global_synth_params;

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
} voice;

void voice_init(voice *v);
void voice_init_global_parameters(voice *v, global_synth_params *sp);
void voice_set_modmatrix_core(voice *v, matrixrow **modmatrix);
void voice_initialize_modmatrix(voice *v, modmatrix *matrix);
bool voice_is_active_voice(voice *v);
bool voice_can_note_off(voice *v);
bool voice_is_voice_done(voice *v);
bool voice_in_legato_mode(voice *v);
void voice_prepare_for_play(voice *v);
void voice_update(voice *v);
void voice_reset(voice *v);
void voice_note_on(voice *v, int midi_note, int midi_velocity, double frequency,
                   double last_note_frequency);
void voice_note_off(voice *v, int midi_note);
bool voice_gennext(voice *v, double *left_output, double *right_output);
void voice_set_sustain_override(voice *v, bool b);

#endif // SBSHELL_VOICE_H_
