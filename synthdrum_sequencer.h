#pragma once

#include "envelope_generator.h"
#include "filter_moogladder.h"
#include "qblimited_oscillator.h"
#include "sound_generator.h"

typedef struct synthdrum_sequencer {
    SOUNDGEN sg;
    qblimited_oscillator m_osc1;
    qblimited_oscillator m_osc2;
    envelope_generator m_eg1;
    envelope_generator m_eg2;
    filter_moogladder m_filter;

} synthdrum_sequencer;

synthdrum_sequencer *new_synthdrum_seq(void);

void sds_status(void *self, wchar_t *ss);
void sds_setvol(void *self, double v);
double sds_gennext(void *self);
double sds_getvol(void *self);
void sds_trigger(synthdrum_sequencer *sds);
void sds_stop(synthdrum_sequencer *sds);
