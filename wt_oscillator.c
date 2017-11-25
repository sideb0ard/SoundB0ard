#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "utils.h"
#include "wt_oscillator.h"

wt_osc *wt_osc_new()
{
    wt_osc *wt = (wt_osc *)calloc(1, sizeof(wt_osc));
    if (wt == NULL)
    {
        printf("Nae mem\n");
        return NULL;
    }

    wt_initialize(wt);

    return wt;
}

void wt_initialize(wt_osc *wt)
{
    osc_new_settings(&wt->osc);

    memset(wt->m_saw_tables, 0, NUM_TABLES * sizeof(double));
    memset(wt->m_tri_tables, 0, NUM_TABLES * sizeof(double));

    wt->m_read_idx = 0;
    wt->m_wt_inc = 0;
    wt->m_read_idx2 = 0;
    wt->m_wt_inc2 = 0;
    wt->m_current_table_idx = 0;

    wt->m_current_table = &wt->m_sine_table[0];

    wt->m_square_corr_factor[0] = 0.5;
    wt->m_square_corr_factor[1] = 0.5;
    wt->m_square_corr_factor[2] = 0.5;
    wt->m_square_corr_factor[3] = 0.49;
    wt->m_square_corr_factor[4] = 0.48;
    wt->m_square_corr_factor[5] = 0.468;
    wt->m_square_corr_factor[6] = 0.43;
    wt->m_square_corr_factor[7] = 0.34;
    wt->m_square_corr_factor[8] = 0.25;

    wt_create_wave_tables(wt);
    wt->osc.do_oscillate = &wt_do_oscillate;
    wt->osc.start_oscillator = &wt_start;
    wt->osc.stop_oscillator = &wt_stop;
    wt->osc.reset_oscillator = &wt_reset;
    wt->osc.update_oscillator = &wt_update; // from base class
}

void wt_reset(oscillator *self)
{
    wt_osc *wt = (wt_osc *)self;

    osc_reset(&wt->osc);
    wt->m_read_idx = 0;
    wt->m_read_idx2 = 0;

    wt_update(self);
}

double wt_do_oscillate(oscillator *self, double *quad_outval)
{
    wt_osc *wt = (wt_osc *)self;

    if (!wt->osc.m_note_on)
    {
        if (quad_outval)
            *quad_outval = 0.0;

        return 0.0;
    }

    if (wt->osc.m_waveform == SQUARE && wt->m_current_table_idx >= 0)
    {
        double out = wt_do_square_wave(wt);
        if (quad_outval)
            *quad_outval = out;
        return out;
    }

    double outval = wt_do_wave_table(wt, &wt->m_read_idx, wt->m_wt_inc);
    if (wt->osc.m_v_modmatrix)
    {
        wt->osc.m_v_modmatrix->m_sources[wt->osc.m_mod_dest_output1] =
            outval * wt->osc.m_amplitude * wt->osc.m_amp_mod;
        wt->osc.m_v_modmatrix->m_sources[wt->osc.m_mod_dest_output2] =
            outval * wt->osc.m_amplitude * wt->osc.m_amp_mod;
    }

    if (quad_outval)
        *quad_outval = outval * wt->osc.m_amplitude * wt->osc.m_amp_mod;

    return outval * wt->osc.m_amplitude * wt->osc.m_amp_mod;
}

double wt_do_wave_table(wt_osc *wt, double *read_idx, double wt_inc)
{
    double out = 0.;
    double mod_read_idx = *read_idx + wt->osc.m_phase_mod * WT_LENGTH;
    wt_check_wrap_index(&mod_read_idx);

    int i_read_idx = abs((int)mod_read_idx);
    float frac = mod_read_idx - i_read_idx;
    int read_idx_next = i_read_idx + 1 > WT_LENGTH - 1 ? 0 : i_read_idx + 1;
    out = lin_terp(0, 1, wt->m_current_table[i_read_idx],
                   wt->m_current_table[read_idx_next], frac);
    *read_idx += wt_inc;

    wt_check_wrap_index(read_idx);
    return out;
}

double wt_do_square_wave(wt_osc *wt)
{
    return wt_do_square_wave_core(wt, &wt->m_read_idx, wt->m_wt_inc);
}

double wt_do_square_wave_core(wt_osc *wt, double *read_idx, double wt_inc)
{
    double pw = wt->osc.m_pulse_width / 100;
    double pwidx = *read_idx + pw * WT_LENGTH;

    double saw1 = wt_do_wave_table(wt, read_idx, wt_inc);

    if (wt_inc >= 0)
    {
        if (pwidx >= WT_LENGTH)
            pwidx = pwidx - WT_LENGTH;
    }
    else
    {
        if (pwidx < 0)
            pwidx = pwidx + WT_LENGTH;
    }

    double saw2 = wt_do_wave_table(wt, &pwidx, wt_inc);

    double sqamp = wt->m_square_corr_factor[wt->m_current_table_idx];
    double out = sqamp * saw1 - sqamp * saw2;

    double corr = 1.0 / pw;
    if (pw < 0.5)
        corr = 1.0 / (1.0 - pw);

    out *= corr;
    return out;
}

void wt_start(oscillator *self)
{
    wt_osc *wt = (wt_osc *)self;
    wt->osc.m_note_on = true;
}

void wt_stop(oscillator *self)
{
    wt_osc *wt = (wt_osc *)self;
    wt->osc.m_note_on = false;
}

void wt_update(oscillator *self)
{
    wt_osc *wt = (wt_osc *)self;
    osc_update(&wt->osc);
    wt->m_wt_inc = (double)WT_LENGTH * wt->osc.m_inc;
    wt_select_table(wt);
}

void wt_select_table(wt_osc *wt)
{
    wt->m_current_table_idx = wt_get_table_index(wt);
    if (wt->m_current_table_idx < 0)
    {
        wt->m_current_table = &wt->m_sine_table[0];
        return;
    }

    if (wt->osc.m_waveform == SAW1 || wt->osc.m_waveform == SAW2 ||
        wt->osc.m_waveform == SAW3 || wt->osc.m_waveform == SQUARE)
        wt->m_current_table = wt->m_saw_tables[wt->m_current_table_idx];
    else if (wt->osc.m_waveform == TRI)
        wt->m_current_table = wt->m_tri_tables[wt->m_current_table_idx];
}

int wt_get_table_index(wt_osc *wt)
{
    if (wt->osc.m_waveform == SINE)
        return -1;

    double seed_freq = 27.5; // A0
    for (int i = 0; i < NUM_TABLES; ++i)
    {
        if (wt->osc.m_fo <= seed_freq)
            return i;

        seed_freq *= 2.0;
    }
    return -1;
}

void wt_create_wave_tables(wt_osc *wt)
{
    for (int i = 0; i < WT_LENGTH; i++)
    {
        wt->m_sine_table[i] = sin((double)i / WT_LENGTH) * (TWO_PI);
    }
    double seed_freq = 27.5; // A0
    for (int i = 0; i < NUM_TABLES; ++i)
    {
        double *saw_table = calloc(WT_LENGTH, sizeof(double));
        double *tri_table = calloc(WT_LENGTH, sizeof(double));
        int harms = (int)((SAMPLE_RATE / 2.0 / seed_freq) - 1.0);
        int half_harms = (int)((float)harms / 2.0);
        double max_saw = 0;
        double max_tri = 0;

        for (int j = 0; j < WT_LENGTH; ++j)
        {
            for (int g = 1; g <= harms; ++g)
            {
                double x = g * M_PI / harms;
                double sigma = sin(x) / x;
                if (g == 1)
                    sigma = 1.0;
                double n = g;
                saw_table[j] += pow((float)-1.0, (float)(g + 1)) * (1.0 / n) *
                                sigma * sin(TWO_PI * j * n / WT_LENGTH);
            }
            for (int g = 1; g <= half_harms; ++g)
            {
                double n = g;
                tri_table[j] += pow((float)-1.0, (float)n) *
                                (1.0 / pow((float)(2 * n + 1), (float)2.0)) *
                                sin(TWO_PI * (2.0 * n + 1) * j / WT_LENGTH);
            }

            if (j == 0)
            {
                max_saw = saw_table[j];
                max_tri = tri_table[j];
            }
            else
            {
                if (saw_table[j] > max_saw)
                    max_saw = saw_table[j];
                if (tri_table[j] > max_tri)
                    max_tri = tri_table[j];
            }
        }
        // normalize
        for (int j = 0; j < WT_LENGTH; ++j)
        {
            saw_table[j] /= max_saw;
            tri_table[j] /= max_tri;
        }

        // store
        wt->m_saw_tables[i] = saw_table;
        wt->m_tri_tables[i] = tri_table;

        seed_freq *= 2.0;
    }
}

void wt_destroy_wave_tables(wt_osc *wt)
{
    for (int i = 0; i < NUM_TABLES; ++i)
    {
        double *p = wt->m_saw_tables[i];
        if (p)
        {
            free(p);
            wt->m_saw_tables[i] = NULL;
        }
        p = wt->m_tri_tables[i];
        if (p)
        {
            free(p);
            wt->m_tri_tables[i] = NULL;
        }
    }
}

inline void wt_check_wrap_index(double *index)
{
    while (*index < 0.0)
        *index += WT_LENGTH;
    while (*index >= WT_LENGTH)
        *index -= WT_LENGTH;
}
