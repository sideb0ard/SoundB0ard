#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "defjams.h"
#include "wt_oscillator.h"
#include "utils.h"

wt_osc *wt_osc_new()
{
    wt_osc *wt = (wt_osc *)calloc(1, sizeof(wt_osc));
    if (wt == NULL) {
        printf("Nae mem\n");
        return NULL;
    }

    osc_new_settings(&wt->osc);
    wt->osc.do_oscillate = &wt_do_oscillate;
    wt->osc.start_oscillator = &wt_start_oscillator;
    wt->osc.stop_oscillator = &wt_stop_oscillator;
    wt->osc.reset_oscillator = &wt_reset_oscillator;
    wt->osc.update_oscillator = &wt_update_oscillator;

    wt->m_square_corr_factor[0] = 0.5;
    wt->m_square_corr_factor[1] = 0.5;
    wt->m_square_corr_factor[2] = 0.5;
    wt->m_square_corr_factor[3] = 0.49;
    wt->m_square_corr_factor[4] = 0.48;
    wt->m_square_corr_factor[5] = 0.468;
    wt->m_square_corr_factor[6] = 0.43;
    wt->m_square_corr_factor[7] = 0.34;
    wt->m_square_corr_factor[8] = 0.25;

    wt->m_current_table = &wt->m_sine_table[0];

    wt_create_wave_tables(wt);

    return wt;
}

// typical overrides
double wt_do_oscillate(oscillator *self, double *aux_output)
{
    if (!self->m_note_on) {
        if (aux_output)
            *aux_output = 0.0;

        return 0.0;
    }
    wt_osc *wt = (wt_osc *) self;

    // if square, it has its own routine
    if (self->m_waveform == SQUARE && wt->m_current_table_index >= 0) {
        double out = wt_do_square_wave(wt);
        if (aux_output)
            *aux_output = out;

        return out;
    }

    // --- get output
    double out_sample = wt_do_wave_table(wt, &wt->m_read_index, wt->m_wt_inc);

    // mono oscillator
    if (aux_output)
        *aux_output = out_sample * self->m_amplitude * self->m_amp_mod;

    return out_sample * self->m_amplitude * self->m_amp_mod;
}

void wt_start_oscillator(oscillator *self)
{
    wt_reset_oscillator(self);
    self->m_note_on = true;
}

void wt_stop_oscillator(oscillator *self) { self->m_note_on = false; }

void wt_reset_oscillator(oscillator *self)
{
    osc_reset(self);
    wt_osc *wt = (wt_osc *)self;
    wt->m_read_index = 0.0;
}

void wt_update_oscillator(oscillator *self)
{
    osc_update(self);
    wt_osc *wt = (wt_osc *)self;
    wt->m_wt_inc = WT_LENGTH * self->m_inc;
    wt_select_table(wt);
}

// get table index based on current self->osc.m_fo
int wt_get_table_index(wt_osc *self)
{
    if (self->osc.m_waveform == SINE)
        return -1;

    double seed_freq = 27.5; // Note A0, bottom of piano
    for (int j = 0; j < NUM_TABLES; j++) {
        if (self->osc.m_fo <= seed_freq) {
            return j;
        }

        seed_freq *= 2.0;
    }

    return -1;
}

void wt_select_table(wt_osc *self)
{
    self->m_current_table_index = wt_get_table_index(self);

    // if the frequency is high enough, the sine table will be returned
    // even for non-sinusoidal waves; anything about 10548 Hz is one
    // harmonic only (sine)
    if (self->m_current_table_index < 0) {
        self->m_current_table = &self->m_sine_table[0];
        return;
    }

    // choose table
    if (self->osc.m_waveform == SAW1 || self->osc.m_waveform == SAW2 || self->osc.m_waveform == SAW3 ||
        self->osc.m_waveform == SQUARE)
        self->m_current_table = self->m_saw_tables[self->m_current_table_index];
    else if (self->osc.m_waveform == TRI)
        self->m_current_table = self->m_triangle_tables[self->m_current_table_index];
}

void wt_create_wave_tables(wt_osc *self)
{
    // create the tables
    //
    // SINE: only need one table
    for (int i = 0; i < WT_LENGTH; i++) {
        // sample the sinusoid, WT_LENGTH points
        // sin(wnT) = sin(2pi*i/WT_LENGTH)
        self->m_sine_table[i] = sin(((double)i / WT_LENGTH) * (2 * M_PI));
    }

    // SAW, TRIANGLE: need 10 tables
    double seed_freq = 27.5; // Note A0, bottom of piano
    for (int j = 0; j < NUM_TABLES; j++) {
        double *saw_table = calloc(WT_LENGTH, sizeof(double));

        double *tri_table = calloc(WT_LENGTH, sizeof(double));

        int nHarms = (int)((SAMPLE_RATE / 2.0 / seed_freq) - 1.0);
        int nHalfHarms = (int)((float)nHarms / 2.0);

        double dMaxSaw = 0;
        double dMaxTri = 0;

        for (int i = 0; i < WT_LENGTH; i++) {
            // sawtooth: += (-1)^g+1(1/g)sin(wnT)
            for (int g = 1; g <= nHarms; g++) {
                // Lanczos Sigma Factor
                double x = g * M_PI / nHarms;
                double sigma = sin(x) / x;

                // only apply to partials above fundamental
                if (g == 1)
                    sigma = 1.0;

                double n = (double)g;
                saw_table[i] += pow((float)-1.0, (float)(g + 1)) * (1.0 / n) *
                                sigma * sin(2.0 * M_PI * i * n / WT_LENGTH);
            }

            // triangle: += (-1)^g(1/(2g+1+^2)sin(w(2n+1)T)
            // NOTE: the limit is nHalfHarms here because of the way the sum is
            // constructed
            // (look at the (2n+1) components
            for (int g = 0; g <= nHalfHarms; g++) {
                double n = (double)g;
                tri_table[i] += pow((float)-1.0, (float)n) *
                                (1.0 / pow((float)(2 * n + 1), (float)2.0)) *
                                sin(2.0 * M_PI * (2.0 * n + 1) * i / WT_LENGTH);
            }

            // store the max values
            if (i == 0) {
                dMaxSaw = saw_table[i];
                dMaxTri = tri_table[i];
            }
            else {
                // test and store
                if (saw_table[i] > dMaxSaw)
                    dMaxSaw = saw_table[i];

                if (tri_table[i] > dMaxTri)
                    dMaxTri = tri_table[i];
            }
        }
        // normalize
        for (int i = 0; i < WT_LENGTH; i++) {
            // normalize it
            saw_table[i] /= dMaxSaw;
            tri_table[i] /= dMaxTri;
        }

        // store
        self->m_saw_tables[j] = saw_table;
        self->m_triangle_tables[j] = tri_table;

        seed_freq *= 2.0;
    }
}

void wt_destroy_wave_tables(wt_osc *self)
{
    for (int i = 0; i < NUM_TABLES; i++) {
        double *p = self->m_saw_tables[i];
        if (p) {
            free(p);
            self->m_saw_tables[i] = 0;
        }

        p = self->m_triangle_tables[i];
        if (p) {
            free(p);
            self->m_triangle_tables[i] = 0;
        }
    }
}

double wt_do_wave_table(wt_osc *self, double *read_index, double wt_inc)
{
    double out = 0;

    // apply phase modulation, if any
    double mo_read_index = *read_index + self->osc.m_phase_mod * WT_LENGTH;

    // check for multi-wrapping on new read index
    wt_check_wrap_index(&mo_read_index);

    // get INT part
    int nReadIndex = abs((int)mo_read_index);

    // get FRAC part
    float fFrac = mo_read_index - nReadIndex;

    // setup second index for interpolation; wrap the buffer if needed
    int nReadIndexNext = nReadIndex + 1 > WT_LENGTH - 1 ? 0 : nReadIndex + 1;

    // interpolate the output
    out = lin_terp(0, 1, self->m_current_table[nReadIndex],
                    self->m_current_table[nReadIndexNext], fFrac);
    // add the increment for next time
    *read_index += wt_inc;

    // check for wrap
    wt_check_wrap_index(read_index);

    return out;
}

double wt_do_square_wave(wt_osc *self)
{
    double dPW = self->osc.m_pulse_width / 100.0;
    double dPWIndex = self->m_read_index + dPW * WT_LENGTH;

    // --- render first sawtooth using read_index
    double dSaw1 = wt_do_wave_table(self, &self->m_read_index, self->m_wt_inc);

    // --- find the phase shifted output
    if (self->m_wt_inc >= 0) {
        if (dPWIndex >= WT_LENGTH)
            dPWIndex = dPWIndex - WT_LENGTH;
    }
    else {
        if (dPWIndex < 0)
            dPWIndex = WT_LENGTH + dPWIndex;
    }

    // --- render second sawtooth using dPWIndex (shifted)
    double dSaw2 = wt_do_wave_table(self, &dPWIndex, self->m_wt_inc);

    // --- find the correction factor from the table
    double dSqAmp = self->m_square_corr_factor[self->m_current_table_index];

    // --- then subtract
    double out = dSqAmp * dSaw1 - dSqAmp * dSaw2;

    // --- calculate the DC correction factor
    double dCorr = 1.0 / dPW;
    if (dPW < 0.5)
        dCorr = 1.0 / (1.0 - dPW);

    // --- apply correction
    out *= dCorr;

    return out;
}

void wt_check_wrap_index(double *index)
{
    while (*index < 0.0)
        *index += WT_LENGTH;
    while (*index >= WT_LENGTH)
        *index -= WT_LENGTH;
}

