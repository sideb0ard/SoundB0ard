#ifndef SBSHELL_VOICE_H_
#define SBSHELL_VOICE_H_

#include "dca.h"
#include "envelope_generator.h"
#include "filter.h"
#include "lfo.h"
#include "modmatrix.h"
#include "oscillator.h"
#include "synthfunctions.h"

typedef struct {

    // shared by source and dest
    modmatrix m_modmatrix;

    bool m_note_on;
    unsigned int m_timestamp;

    unsigned int m_midi_note_number;
    unsigned int m_midi_note_number_pending;
    unsigned int m_midi_velocity;
    unsigned int m_midi_velocity_pending;

    /////////////////////////////
    oscillator *m_osc1;
    oscillator *m_osc2;
    oscillator *m_osc3;
    oscillator *m_osc4;

    filter *m_filter1;
    filter *m_filter2;

    envelope_generator *m_eg1;
    envelope_generator *m_eg2;
    envelope_generator *m_eg3;
    envelope_generator *m_eg4;

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

    double m_portamento_time_msec;
    double m_portamento_start;

    double m_modulo_portamento;
    double m_portamento_inc;

    double m_portamento_semitones;

    bool m_note_pending;

    double m_default_mod_intensity;
    double m_default_mod_range;
} voice;

voice *new_voice(void);
void voice_init_global_parameters(voice *v, global_synth_params *sp);
void voice_set_modmatrix_core(voice *v, matrixrow **modmatrix);
void voice_initialize_modmatrix(voice *v, modmatrix *matrix);
bool voice_is_active(voice *v);
bool voice_can_note_off(voice *v);
bool voice_is_voice_done(voice *v);
bool voice_in_legato_mode(voice *v);
void voice_note_on(voice *v, unsigned int midi_note, unsigned int midi_velocity,
                   double frequency, double last_note_frequency);
void voice_note_off(voice *v, unsigned int midi_note);
bool voice_gennext(voice *v, double *left_output, double *right_output);

#endif // SBSHELL_VOICE_H_
