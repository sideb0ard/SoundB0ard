#pragma once

#include <stdbool.h>

#include "defjams.h"
#include "filter_moogladder.h"
#include "fx.h"
#include "lfo.h"

enum
{
    LOWPASS,
    HIGHPASS,
    ALLPASS,
    BANDPASS
};

typedef struct filterpass
{
    fx m_fx; // API
    filter_moogladder m_filter;
    lfo m_lfo1; // route to freq
    bool m_lfo1_active;
    lfo m_lfo2; // route to qv
    bool m_lfo2_active;
} filterpass;

filterpass *new_filterpass(void);

void filterpass_status(void *self, char *string);
double filterpass_process_audio(void *self, double input);

void filterpass_set_lfo_active(filterpass *fp, int lfo_num, bool b);
void filterpass_set_lfo_rate(filterpass *fp, int lfo_num, double val);
void filterpass_set_lfo_amp(filterpass *fp, int lfo_num, double val);
void filterpass_set_lfo_type(filterpass *fp, int lfo_num, unsigned int type);
