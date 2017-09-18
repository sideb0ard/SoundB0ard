#pragma once

#include "filter_sem.h"
#include "sample_oscillator.h"
#include "voice.h"

typedef struct digisynth_voice
{
    voice m_voice;

    sampleosc m_osc1;
    filter_sem m_filter;

} digisynth_voice;

void digisynth_voice_init(digisynth_voice *dv, char *filename);
void digisynth_voice_open_wav(digisynth_voice *dv, char *filename);
bool digisynth_voice_gennext(digisynth_voice *dv, double *out_left,
                             double *out_right);
