#pragma once

#include "defjams.h"
#include "modmatrix.h"
#include "synthfunctions.h"

// 46.88.. = semitones between frequencies (80, 18000.0) / 2
// taken from Will Pirkle book 'designing software synths..'
#define FILTER_FC_MOD_RANGE 46.881879936465680
#define FILTER_FC_MIN 180        // 80Hz
#define FILTER_FC_MAX 18000     // 18 kHz
#define FILTER_FC_DEFAULT 12000 // 10kHz
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

typedef struct filter filter;

struct filter {
    modmatrix *g_modmatrix;

    // sources
    unsigned m_mod_source_fc;
    unsigned m_mod_source_fc_control;

    global_filter_params *m_global_filter_params;

    // GUI controls
    double m_fc_control;  // filter cut-off
    double m_q_control;   // 'qualvity factor' 1-10
    double m_aux_control; // a spare control, used in SEM and ladder filters

    unsigned m_nlp;      // Non Linear Processing on/off switch
    double m_saturation; // used in NLP

    unsigned m_filter_type;

    double m_fc;     // current filter cut-off val
    double m_q;      // current q value
    double m_fc_mod; // frequency cutoff modulation input

    void (*set_fc_mod)(filter *self, double d);
    void (*set_q_control)(filter *self, double d);
    void (*update)(filter *self);
    void (*reset)(filter *self);

    double (*gennext)(filter *self, double xn); // do_filter
};

void filter_setup(filter *self);

void filter_set_fc_mod(filter *self, double val);
void filter_set_q_control(filter *self, double val);
void filter_update(filter *self);
void filter_reset(filter *self);
void filter_init_global_parameters(filter *self, global_filter_params *params);
