#pragma once

#include "ddlmodule.h"
#include "fx.h"
#include "wt_oscillator.h"

typedef enum
{
    FLANGER,
    VIBRATO,
    CHORUS,
    MAX_MOD_TYPE
} modular_type;

typedef struct mod_delay
{
    fx m_fx;
    wt_oscillator m_lfo;
    ddlmodule m_ddl;

    double m_min_delay_msec;
    double m_max_delay_msec;

    double m_mod_depth_pct;
    double m_mod_freq;
    double m_feedback_percent;
    double m_chorus_offset;

    unsigned int m_mod_type;  // FLANGER, VIBRATO, CHORUS
    unsigned int m_lfo_type;  // TRI / SINE
    unsigned int m_lfo_phase; // NORMAL / QUAD / INVERT

} mod_delay;

mod_delay *new_mod_delay(void);
void mod_delay_init(mod_delay *md);
void mod_delay_update_lfo(mod_delay *md);
void mod_delay_update_ddl(mod_delay *md);
void mod_delay_cook_mod_type(mod_delay *md);
double mod_delay_calculate_delay_offset(mod_delay *md, double lfo_sample);
bool mod_delay_prepare_for_play(mod_delay *md);
bool mod_delay_update(mod_delay *md);
bool mod_delay_process_audio(mod_delay *md, double *input_left,
                             double *input_right, double *output_left,
                             double *output_right);

double mod_delay_process_wrapper(void *self, double input);
void mod_delay_status(void *self, char *status_string);

void mod_delay_set_depth(mod_delay *md, double val);
void mod_delay_set_rate(mod_delay *md, double val);
void mod_delay_set_feedback_percent(mod_delay *md, double val);
void mod_delay_set_chorus_offset(mod_delay *md, double val);
void mod_delay_set_mod_type(mod_delay *md, unsigned int val);
void mod_delay_set_lfo_type(mod_delay *md, unsigned int val);
