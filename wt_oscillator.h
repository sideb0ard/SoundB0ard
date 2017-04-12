#pragma once
#include "oscillator.h"

#define WT_LENGTH 512
#define NUM_TABLES 9

typedef struct wt_oscillator wt_osc;

struct wt_oscillator {

    oscillator osc;

    double m_read_index;
    double m_wt_inc;

    double m_sine_table[WT_LENGTH];
    double *m_saw_tables[NUM_TABLES];
    double *m_triangle_tables[NUM_TABLES];

    // for storing current table
    double *m_current_table;
    int m_current_table_index; // 0 - 9

    // correction factor table sum-of-sawtooth
    double m_square_corr_factor[NUM_TABLES];
};

wt_osc *wt_osc_new(void);

void wt_prepare(wt_osc *osc);

// typical overrides
double wt_do_oscillate(oscillator *self, double *aux_output);
void wt_start_oscillator(oscillator *self);
void wt_stop_oscillator(oscillator *self);
void wt_reset_oscillator(oscillator *self);

void wt_update_oscillator(oscillator *self, char *name);
// find the table with the proper number of harmonics for our pitch
int wt_get_table_index(wt_osc *self);
void wt_select_table(wt_osc *self);

// create an destory tables
void wt_create_wave_tables(wt_osc *self);
void wt_destroy_wave_tables(wt_osc *self);

// do the selected wavetable
double wt_do_wave_table(wt_osc *self, double *read_index, double wt_inc);

// for square wave
double wt_do_square_wave(wt_osc *self);

void wt_check_wrap_index(double *index);
