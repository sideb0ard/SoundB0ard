#pragma once

#include "lfo.h"
#include "ddlmodule.h"

typedef enum { FLANGER, VIBRATO, CHORUS } modular_type;

typedef struct mod_delay {
    lfo *m_lfo;
    ddlmodule m_delay;

    double m_min_delay_msec;
    double m_max_delay_msec;

    double m_depth;
    double m_rate;
    double m_feedback_percent;
    double m_chorus_offset;

    unsigned int m_mod_type; // FLANGER, VIBRATO, CHORUS
    unsigned int m_lfo_type; // TRI / SIN

} mod_delay;

mod_delay *new_mod_delay(void);
void mod_delay_init(mod_delay *md);
void mod_delay_update_lfo(mod_delay *md);
void mod_delay_update_delay(mod_delay *md);
void mod_delay_cook_mod_type(mod_delay *md);
double mod_delay_calculate_delay_offset(mod_delay *md, double lfo_sample);
bool mod_delay_prepare_for_play(mod_delay *md);
bool mod_delay_update(mod_delay *md);
bool mod_delay_process_audio(mod_delay *md, double *input_left,
                             double *input_right, double *output_left,
                             double *output_right);
