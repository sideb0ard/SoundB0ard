#pragma once

#include "filter_ckthreefive.h"
#include "filter_moogladder.h"
#include "qblimited_oscillator.h"
#include "voice.h"
#include "wt_oscillator.h"

typedef enum {
    Saw3,
    Sqr3,
    Saw2Sqr,
    Tri2Saw,
    Tri2Sqr,
    Sin2Sqr,
    MAX_VOICE_CHOICE
} minisynth_voice_choice;

typedef struct
{
    voice m_voice;

    qblimited_oscillator m_osc1;
    qblimited_oscillator m_osc2;
    qblimited_oscillator m_osc3;
    qblimited_oscillator m_osc4;
    // wt_oscillator m_osc1;
    // wt_oscillator m_osc2;
    // wt_oscillator m_osc3;
    // wt_oscillator m_osc4;

    filter_moogladder m_filter;
    // filter_ckthreefive m_filter;

} minisynth_voice;

minisynth_voice *new_minisynth_voice(void);
void minisynth_voice_init(minisynth_voice *ms);
void minisynth_voice_free_self(minisynth_voice *msv);

void minisynth_voice_init_global_parameters(minisynth_voice *ms,
                                            global_synth_params *sp);
void minisynth_voice_initialize_modmatrix(minisynth_voice *ms,
                                          modmatrix *matrix);
void minisynth_voice_prepare_for_play(minisynth_voice *ms);
void minisynth_voice_update(minisynth_voice *ms);
void minisynth_voice_reset(minisynth_voice *ms);

bool minisynth_voice_gennext(minisynth_voice *ms, double *left_output,
                             double *right_output);
void minisynth_voice_set_filter_mod(minisynth_voice *ms, double mod);
