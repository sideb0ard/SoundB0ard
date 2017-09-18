#pragma once

#include "wt_oscillator.h"
// #include "filter_moogladder.h"
// #include "lfo.h"
// #include "qblimited_oscillator.h"
// #include "reverb.h"
#include "sound_generator.h"

typedef struct spork
{
    soundgenerator sg;
    wt_oscillator m_osc;
    // lfo m_lfo;
    // qblimited_oscillator m_osc1;
    // qblimited_oscillator m_osc2;
    // qblimited_oscillator m_osc3;
    // filter_moogladder m_filter;

    // reverb *m_reverb;

    // "gui"
    double freq;
    unsigned int waveform;
    unsigned int mode;
    unsigned int polarity;

    bool active;
    double m_volume;
} spork;

spork *new_spork(double freq);
double spork_gennext(void *sg);

void spork_status(void *self, wchar_t *ss);
double spork_getvol(void *self);
void spork_setvol(void *self, double v);
void spork_start(void *self);
void spork_stop(void *self);

void spork_set_freq(spork *s, double freq);
void spork_set_waveform(spork *s, unsigned int wave);
void spork_set_mode(spork *s, unsigned int mode);
void spork_set_polarity(spork *s, unsigned int polarity);
void spork_update(spork *s);
