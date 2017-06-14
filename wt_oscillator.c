#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "utils.h"
#include "wt_oscillator.h"

wt_osc *wt_osc_new()
{
    wt_osc *wt = (wt_osc *)calloc(1, sizeof(wt_osc));
    if (wt == NULL) {
        printf("Nae mem\n");
        return NULL;
    }

    wt_init(wt);

    return wt;
}

void wt_init(wt_osc *wt)
{
    wt_create_wave_tables(wt);
    wt_reset(wt);
}

void wt_reset(wt_osc *wt)
{
    wt->m_read_index = 0;
    wt->m_inc = 0;
    wt->noteon = false;
    wt->freq = 440;
    wt->waveform = 0;
    wt->mode = 0;
    wt->polarity = 0;
    wt_update(wt);
}

// typical overrides
double wt_do_oscillate(wt_osc *wt)
{
    if (!wt->noteon) {
        return 0.0;
    }
    int read_index = (int) wt->m_read_index;
    double frac = read_index - wt->m_read_index;
    int read_index_next = read_index + 1 > WT_LENGTH ? 0 : read_index + 1;

    double outval = 0.0;

    switch(wt->waveform) {
    case(0): // sine
        outval = lin_terp(0, 1,
                          wt->m_sine_array[read_index],
                          wt->m_sine_array[read_index_next],
                          frac);
        break;
    case(1): // saw
        outval = lin_terp(0, 1,
                          wt->m_saw_array[read_index],
                          wt->m_saw_array[read_index_next],
                          frac);
        break;
    case(2): // tri
        outval = lin_terp(0, 1,
                          wt->m_triangle_array[read_index],
                          wt->m_triangle_array[read_index_next],
                          frac);
        break;
    case(3): // square
        outval = lin_terp(0, 1,
                          wt->m_square_array[read_index],
                          wt->m_square_array[read_index_next],
                          frac);
        break;
    default:
        outval = lin_terp(0, 1,
                          wt->m_sine_array[read_index],
                          wt->m_sine_array[read_index_next],
                          frac);
    }

    wt->m_read_index += wt->m_inc;
    wt_check_wrap_index(&wt->m_read_index);

    return outval;
                       

}

void wt_start(wt_osc *wt)
{
    wt->noteon = true;
}

void wt_stop(wt_osc *wt) { wt->noteon = false; }

void wt_update(wt_osc *wt)
{
    wt->m_inc = (double) WT_LENGTH * wt->freq / (double)SAMPLE_RATE;
}


void wt_create_wave_tables(wt_osc *wt)
{
    //tri 
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


    for (int i = 0; i < WT_LENGTH; i++) {
        wt->m_sine_array[i] = sin(((double)i / WT_LENGTH) * (2 * M_PI));
        wt->m_saw_array[i] = i < 512 ? ms1*i + bs1 : ms2*(i-511) + bs2;
        wt->m_square_array[i] = i < 512 ? 1.0 : -1.0;

        if (i < 256)
            wt->m_triangle_array[i] = mt1 * i + bt1;
        else if (i >= 256 && i < 768)
            wt->m_triangle_array[i] = mtf2 * ( i - 256) + btf2;
        else
            wt->m_triangle_array[i] = mt2 * ( i - 768) + bt2;

    }
}

void wt_check_wrap_index(double *index)
{
    while (*index < 0.0)
        *index += WT_LENGTH;
    while (*index >= WT_LENGTH)
        *index -= WT_LENGTH;
}
