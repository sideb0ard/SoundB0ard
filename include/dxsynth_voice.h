#pragma once

#include "qblimited_oscillator.h"
#include "voice.h"
#include "wt_oscillator.h"

enum
{
    DX_LFO_DEST_NONE,
    DX_LFO_DEST_AMP_MOD,
    DX_LFO_DEST_VIBRATO
};

typedef struct
{
    voice m_voice;

    qblimited_oscillator m_op1;
    qblimited_oscillator m_op2;
    qblimited_oscillator m_op3;
    qblimited_oscillator m_op4;
    // wt_oscillator m_op1;
    // wt_oscillator m_op2;
    // wt_oscillator m_op3;
    // wt_oscillator m_op4;

    double m_op1_feedback;
    double m_op2_feedback;
    double m_op3_feedback;
    double m_op4_feedback;

    unsigned int m_lfo_mod_dest; // none, ampmod, or vibrato

} dxsynth_voice;

dxsynth_voice *new_dxsynth_voice(void);
void dxsynth_voice_init(dxsynth_voice *ms);
void dxsynth_voice_free_self(dxsynth_voice *msv);

void dxsynth_voice_init_global_parameters(dxsynth_voice *ms,
                                          global_synth_params *sp);
void dxsynth_voice_initialize_modmatrix(dxsynth_voice *ms, modmatrix *matrix);
void dxsynth_voice_prepare_for_play(dxsynth_voice *ms);
void dxsynth_voice_update(dxsynth_voice *ms);
void dxsynth_voice_reset(dxsynth_voice *ms);

bool dxsynth_voice_gennext(dxsynth_voice *ms, double *left_output,
                           double *right_output);
bool dxsynth_voice_can_note_off(dxsynth_voice *dxv);
bool dxsynth_voice_is_voice_done(dxsynth_voice *dxv);
void dxsynth_voice_set_lfo1_dest(dxsynth_voice *ms, unsigned int op,
                                 unsigned int dest);
void dxsynth_voice_set_output_egs(dxsynth_voice *ms);
