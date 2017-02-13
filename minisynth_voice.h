#pragma once

#include "voice.h"
#include "qblimited_oscillator.h"
#include "filter_moogladder.h"

typedef enum {Saw3,Sqr3,Saw2Sqr,Tri2Saw,Tri2Sqr} minisynth_voice_choice;

typedef struct
{
    voice m_voice;

    qblimited_oscillator m_osc1;
    qblimited_oscillator m_osc2;
    qblimited_oscillator m_osc3;
    qblimited_oscillator m_osc4;

    filter_moogladder m_moog_ladder_filter;

} minisynth_voice;

minisynth_voice* new_minisynth_voice(void);

void minisynth_voice_init_global_parameters(minisynth_voice *msv, global_synth_params *sp);
void minisynth_voice_initialize_modmatrix(minisynth_voice *msv, modmatrix *matrix);
void minisynth_voice_prepare_for_play(minisynth_voice *msv);
void minisynth_voice_update(minisynth_voice *msv);
void minisynth_voice_reset(minisynth_voice *msv);

bool minisynth_voice_gennext(minisynth_voice *msv, double *left_output, double *right_output);
