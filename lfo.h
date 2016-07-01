#pragma once

#include "defjams.h"

typedef struct lfo {

    double m_freq;
    double m_curphase;
    double m_incr;
    double m_amp;

    oscil_type m_type;

} LFO;

LFO *new_lfo(double freq, oscil_type type);

double lfo_gennext(LFO *self);
void lfo_change_freq(LFO *self, double freq);
