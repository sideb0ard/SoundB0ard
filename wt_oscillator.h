#pragma once
#include "oscillator.h"

#define WT_LENGTH 1024
#define NUM_TABLES 9

typedef struct wt_oscillator wt_osc;
typedef struct wt_oscillator
{

    double m_read_index;
    double m_quad_phase_read_index;

    double m_inc;

    bool m_invert;

    bool noteon;

    double freq;
    unsigned int waveform; // 0: sine, saw, tri, square
    unsigned int mode;     // 0: normal, bandlimited
    unsigned int polarity; // bipolar, unipolar

    double m_sine_array[WT_LENGTH];

    double m_saw_array[WT_LENGTH];
    double m_triangle_array[WT_LENGTH];
    double m_square_array[WT_LENGTH];

    double m_saw_array_bl5[WT_LENGTH]; // band limited to 5 partials
    double m_triangle_array_bl5[WT_LENGTH];
    double m_square_array_bl5[WT_LENGTH];

} wt_oscillator;

wt_osc *wt_osc_new(void);

void wt_initialize(wt_osc *wt);

double wt_do_oscillate(wt_osc *wt, double *quad_output);
void wt_start(wt_osc *wt);
void wt_stop(wt_osc *wt);
void wt_reset(wt_osc *wt);
void wt_update(wt_osc *wt);
void wt_create_wave_tables(wt_osc *wt);

void wt_check_wrap_index(double *index);
