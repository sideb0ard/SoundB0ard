#pragma once

#include "lfo.h"
#include "sound_generator.h"
#include "qblimited_oscillator.h"
#include "filter_moogladder.h"

typedef struct spork {
    SOUNDGEN sg;
    lfo m_lfo;
    qblimited_oscillator m_osc1;
    qblimited_oscillator m_osc2;
    qblimited_oscillator m_osc3;
    filter_moogladder m_filter;

    double m_volume;
} spork;

spork *new_spork(void);
double spork_gennext(void *sg);

void spork_status(void *self, wchar_t *ss);
double spork_getvol(void *self);
void spork_setvol(void *self, double v);
