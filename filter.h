#pragma once

#include "defjams.h"
#include "modmatrix.h"

// 46.88.. = semitones between frequencies (80, 18000.0) / 2
// taken from Will Pirkle book 'designing software synths..'
#define FILTER_FC_MOD_RANGE 46.881879936465680
#define FILTER_FC_MIN 80        // 80Hz
#define FILTER_FC_MAX 18000     // 18 kHz
#define FILTER_FC_DEFAULT 10000 // 10kHz
#define FILTER_Q_DEFAULT 0.707  // butterworth?
#define FILTER_TYPE_DEFAULT LPF1

typedef enum {
    LPF1,
    HPF1,
    LPF2,
    HPF2,
    BPF2,
    BSF2,
    LPF4,
    HPF4,
    BPF4
} filter_type;

typedef struct filter {
    // GUI controls
    double m_fc_control;  // filter cut-off
    double m_q_control;   // 'qualvity factor' 1-10
    double m_aux_control; // a spare control, used in SEM and ladder filters

    double m_saturation; // used in NLP

    filter_type m_type;
    onoff m_nlp; // Non Linear Processing on/off switch

    double m_fc;     // current filter cut-off val
    double m_q;      // current q value
    double m_fc_mod; // frequency cutoff modulation input

    modmatrix *global_modmatrix;
    // sources
    unsigned m_mod_source_fc;
    unsigned m_mod_source_fc_control;

    double (*gennext)(void *self, double xn);
    void (*update)(void *self);
    void (*reset)(void *self);

} FILTER;

FILTER *new_filter(void);
void filter_set_q_control(void *self, double val);
void filter_update(void *self);
void filter_set_fc_mod(void *self, double val);
void filter_adj_fc_control(void *filter, int direction);
void filter_set_fc_control(void *self, double val);
