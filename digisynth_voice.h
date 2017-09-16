#pragma once

#include "filter_sem.h"
#include "sample_oscillator.h"

typedef struct digisynth_voice
{
    sampleosc *m_osc1;
    filter_sem *m_left_filter;
    filter_sem *m_right_filter;
} digisynth_voice;
