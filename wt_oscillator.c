#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
    wt_create_wave_tables(wt);
    wt_reset(wt);
    wt_update(wt);
}

void wt_reset(wt_osc *wt)
{
    wt->m_read_index = 0;
    wt->m_quad_phase_read_index = WT_LENGTH / 4;
    wt->m_inc = 0;
    wt->noteon = false;
    wt->m_invert = false;
    wt->freq = 440;
    wt->waveform = 0;
    wt->mode = 0;
    wt->polarity = 0;
    wt_update(wt);
}

double wt_do_oscillate(wt_osc *wt, double *quad_outval)
{
    if (!wt->noteon)
    {
        return 0.0;
        *quad_outval = 0.0;
    }
    int read_index = (int)wt->m_read_index;
    int quad_read_index = (int)wt->m_quad_phase_read_index;
    double frac = read_index - wt->m_read_index;
    int read_index_next = read_index + 1 > (WT_LENGTH - 1) ? 0 : read_index + 1;
    int quad_read_index_next =
        quad_read_index + 1 > (WT_LENGTH - 1) ? 0 : quad_read_index + 1;

    double outval = 0.0;
    *quad_outval = 0.0;

    switch (wt->waveform)
    {
    case (0): // sine
        outval = lin_terp(0, 1, wt->m_sine_array[read_index],
                          wt->m_sine_array[read_index_next], frac);
        *quad_outval = lin_terp(0, 1, wt->m_sine_array[quad_read_index],
                                wt->m_sine_array[quad_read_index_next], frac);
        break;
    case (1):              // saw
        if (wt->mode == 0) // normal
        {
            outval = lin_terp(0, 1, wt->m_saw_array[read_index],
                              wt->m_saw_array[read_index_next], frac);
            *quad_outval =
                lin_terp(0, 1, wt->m_saw_array[quad_read_index],
                         wt->m_saw_array[quad_read_index_next], frac);
        }
        else
        {
            outval = lin_terp(0, 1, wt->m_saw_array_bl5[read_index],
                              wt->m_saw_array_bl5[read_index_next], frac);
            *quad_outval =
                lin_terp(0, 1, wt->m_saw_array_bl5[quad_read_index],
                         wt->m_saw_array_bl5[quad_read_index_next], frac);
        }
        break;
    case (2):              // tri
        if (wt->mode == 0) // normal
        {
            outval = lin_terp(0, 1, wt->m_triangle_array[read_index],
                              wt->m_triangle_array[read_index_next], frac);
            *quad_outval =
                lin_terp(0, 1, wt->m_triangle_array[quad_read_index],
                         wt->m_triangle_array[quad_read_index_next], frac);
        }
        else
        {
            outval = lin_terp(0, 1, wt->m_triangle_array_bl5[read_index],
                              wt->m_triangle_array_bl5[read_index_next], frac);
            *quad_outval =
                lin_terp(0, 1, wt->m_triangle_array_bl5[quad_read_index],
                         wt->m_triangle_array_bl5[quad_read_index_next], frac);
        }
        break;
    case (3):              // square
        if (wt->mode == 0) // normal
        {
            outval = lin_terp(0, 1, wt->m_square_array[read_index],
                              wt->m_square_array[read_index_next], frac);
            *quad_outval =
                lin_terp(0, 1, wt->m_square_array[quad_read_index],
                         wt->m_square_array[quad_read_index_next], frac);
        }
        else
        {
            outval = lin_terp(0, 1, wt->m_square_array_bl5[read_index],
                              wt->m_square_array_bl5[read_index_next], frac);
            *quad_outval =
                lin_terp(0, 1, wt->m_square_array_bl5[quad_read_index],
                         wt->m_square_array_bl5[quad_read_index_next], frac);
        }
        break;
    default:
        outval = lin_terp(0, 1, wt->m_sine_array[read_index],
                          wt->m_sine_array[read_index_next], frac);
        *quad_outval = lin_terp(0, 1, wt->m_sine_array[quad_read_index],
                                wt->m_sine_array[quad_read_index_next], frac);
    }

    if (wt->m_invert)
    {
        outval *= -1.0;
        *quad_outval *= -1.0;
    }

    if (wt->polarity == 1)
    { // bipolar
        outval /= 2.0;
        outval += 0.5;

        *quad_outval /= 2.0;
        *quad_outval += 0.5;
    }

    wt->m_read_index += wt->m_inc;
    wt->m_quad_phase_read_index += wt->m_inc;
    wt_check_wrap_index(&wt->m_read_index);
    wt_check_wrap_index(&wt->m_quad_phase_read_index);

    return outval;
}

void wt_start(wt_osc *wt) { wt->noteon = true; }

void wt_stop(wt_osc *wt) { wt->noteon = false; }

void wt_update(wt_osc *wt)
{
    wt->m_inc = (double)WT_LENGTH * wt->freq / (double)SAMPLE_RATE;
}

void wt_create_wave_tables(wt_osc *wt)
{
    // tri
    double mt1 = 1.0 / 256.0;
    double bt1 = 0.0;

    double mt2 = 1.0 / 256.;
    double bt2 = -1.0;

    double mtf2 = -2.0 / 512.;
    double btf2 = 1.;

    // sawtooth
    double ms1 = 1.0 / 512.;
    double bs1 = 0.0;

    double ms2 = 1.0 / 512.;
    double bs2 = -1.0;

    double max_tri = 0.;
    double max_saw = 0.;
    double max_sqr = 0.;

    for (int i = 0; i < WT_LENGTH; i++)
    {
        wt->m_sine_array[i] = sin(((double)i / WT_LENGTH) * (2 * M_PI));
        wt->m_saw_array[i] = i < 512 ? ms1 * i + bs1 : ms2 * (i - 511) + bs2;

        if (i < 256)
            wt->m_triangle_array[i] = mt1 * i + bt1;
        else if (i >= 256 && i < 768)
            wt->m_triangle_array[i] = mtf2 * (i - 256) + btf2;
        else
            wt->m_triangle_array[i] = mt2 * (i - 768) + bt2;

        wt->m_square_array[i] = i < 512 ? 1.0 : -1.0;

        wt->m_saw_array_bl5[i] = 0.;
        wt->m_square_array_bl5[i] = 0.;
        wt->m_triangle_array_bl5[i] = 0.;

        for (int g = 1; g <= 6; g++)
        {
            double n = (double)g;
            wt->m_saw_array_bl5[i] += pow((float)-1.0, (float)(g + 1)) *
                                      (1.0 / n) *
                                      sin(2.0 * M_PI * i * n / WT_LENGTH);
        }

        for (int g = 0; g <= 3; g++)
        {
            double n = (double)g;
            wt->m_triangle_array_bl5[i] +=
                pow((float)-1.0, (float)n) *
                (1.0 / pow((float)(2 * n + 1), (float)2.0)) *
                sin(2.0 * M_PI * (2.0 * n + 1) * i / (float)WT_LENGTH);
        }

        for (int g = 1; g <= 5; g += 2)
        {
            double n = (double)g;
            wt->m_square_array_bl5[i] +=
                (1.0 / n) * sin(2.0 * M_PI * i * n / (float)WT_LENGTH);
        }

        if (i == 0)
        {
            max_saw = wt->m_saw_array_bl5[i];
            max_tri = wt->m_triangle_array_bl5[i];
            max_sqr = wt->m_square_array_bl5[i];
        }
        else
        {
            if (wt->m_saw_array_bl5[i] > max_saw)
                max_saw = wt->m_saw_array_bl5[i];
            if (wt->m_triangle_array_bl5[i] > max_tri)
                max_tri = wt->m_triangle_array_bl5[i];
            if (wt->m_square_array_bl5[i] > max_sqr)
                max_sqr = wt->m_square_array_bl5[i];
        }
    }
    for (int i = 0; i < WT_LENGTH; i++)
    {
        wt->m_saw_array_bl5[i] /= max_saw;
        wt->m_triangle_array_bl5[i] /= max_tri;
        wt->m_square_array_bl5[i] /= max_sqr;
    }
}

void wt_check_wrap_index(double *index)
{
    while (*index < 0.0)
        *index += WT_LENGTH;
    while (*index >= WT_LENGTH)
        *index -= WT_LENGTH;
}
