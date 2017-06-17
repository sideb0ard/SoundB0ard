#pragma once

#include "modular_delay.h"

typedef struct quad_flanger {
    fx m_fx;
    modular_delay m_moddelay_left;
    modular_delay m_moddelay_right;

    double m_min_delay_msec;
    double m_max_delay_msec;

    double m_mod_depth_pct;
    double m_mod_freq;
    double m_feedback_percent;
    unsigned int m_lfo_type; // TRI / SINE

} quad_flanger;

quad_flanger *new_quad_flanger(void);
void quad_flanger_init(quad_flanger *qf);
bool quad_flanger_prepare_for_play(quad_flanger *qf);
bool quad_flanger_update_mod_delays(quad_flanger *qf);
bool quad_flanger_update(quad_flanger *qf);
bool quad_flanger_process_audio(quad_flanger *qf, double *input_left,
                                double *input_right, double *output_left,
                                double *output_right);

double quad_flanger_process_wrapper(void *self, double input);
void quad_flanger_status(void *self, char *status_string);

void quad_flanger_set_depth(quad_flanger *qf, double val);
void quad_flanger_set_rate(quad_flanger *qf, double val);
void quad_flanger_set_feedback_percent(quad_flanger *qf, double val);
void quad_flanger_set_lfo_type(quad_flanger *qf, unsigned int val);
