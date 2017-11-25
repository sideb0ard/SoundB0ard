#pragma once
#include "oscillator.h"

#define WT_LENGTH 1024
#define NUM_TABLES 9

typedef struct wt_oscillator wt_osc;
typedef struct wt_oscillator
{
    oscillator osc;

    double m_read_idx;
    double m_wt_inc;

    double m_read_idx2;
    double m_wt_inc2;

    double m_sine_table[WT_LENGTH];
    double *m_saw_tables[NUM_TABLES];
    double *m_tri_tables[NUM_TABLES];

    double *m_current_table;
    int m_current_table_idx;

    // correction factor
    double m_square_corr_factor[NUM_TABLES];

} wt_oscillator;

wt_osc *wt_osc_new(void);

void wt_initialize(wt_osc *wt);

void wt_start(oscillator *self);
void wt_stop(oscillator *self);
void wt_reset(oscillator *self);
void wt_update(oscillator *self);
double wt_do_oscillate(oscillator *self, double *quad_output);

double wt_do_wave_table(wt_osc *wt, double *read_idx, double wt_inc);
double wt_do_square_wave(wt_osc *wt);
double wt_do_square_wave_core(wt_osc *wt, double *read_idx, double wt_inc);

void wt_check_wrap_index(double *index);
void wt_select_table(wt_osc *wt);
int wt_get_table_index(wt_osc *wt);

void wt_create_wave_tables(wt_osc *wt);
void wt_destroy_wave_tables(wt_osc *wt);
