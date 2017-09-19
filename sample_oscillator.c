#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "midi_freq_table.h"
#include "sample_oscillator.h"
#include "utils.h"

void sampleosc_init(sampleosc *sosc, char *filename)
{
    audiofile_data_import_file_contents(&sosc->afd, filename);
    sampleosc_set_oscillator_interface(sosc);
    sosc->is_single_cycle = false;
    sosc->is_pitchless = false;
    sosc->loop_mode = SAMPLE_LOOP;
    osc_new_settings(&sosc->osc);
}

void sampleosc_set_oscillator_interface(sampleosc *sosc)
{
    sosc->osc.do_oscillate = &sampleosc_do_oscillate;
    sosc->osc.start_oscillator = &sampleosc_start_oscillator;
    sosc->osc.stop_oscillator = &sampleosc_stop_oscillator;
    sosc->osc.reset_oscillator = &sampleosc_reset_oscillator;
    sosc->osc.update_oscillator = &sampleosc_update;
}

void sampleosc_start_oscillator(oscillator *self)
{
    sampleosc_reset_oscillator(self);
    self->m_note_on = true;
}
void sampleosc_stop_oscillator(oscillator *self) { self->m_note_on = false; }

void sampleosc_reset_oscillator(oscillator *self)
{
    sampleosc *sosc = (sampleosc *)self;
    osc_reset(&sosc->osc);
    sosc->m_read_idx = 0;
    sampleosc_update((oscillator *)sosc);
}

double sampleosc_read_sample_buffer(sampleosc *sosc)
{
    double return_val = 0;

    int read_idx = sosc->m_read_idx;
    double frac = sosc->m_read_idx - read_idx;

    if (sosc->afd.channels == 1)
    {
        int next_read_idx =
            read_idx + 1 > sosc->afd.samplecount - 1 ? 0 : read_idx + 1;

        return_val = lin_terp(0, 1, sosc->afd.filecontents[read_idx],
                              sosc->afd.filecontents[next_read_idx], frac);

        sosc->m_read_idx += sosc->osc.m_inc;
    }

    else if (sosc->afd.channels == 2)
    {
        int read_idx_left = (int)sosc->m_read_idx * 2;
        int next_read_idx_left = read_idx_left + 2 > sosc->afd.samplecount - 1
                                     ? 0
                                     : read_idx_left + 2;
        double left_sample =
            lin_terp(0, 1, sosc->afd.filecontents[read_idx_left],
                     sosc->afd.filecontents[next_read_idx_left], frac);

        int read_idx_right = read_idx_left + 1;
        int next_read_idx_right = read_idx_right + 2 > sosc->afd.samplecount - 1
                                      ? 1
                                      : read_idx_right + 2;
        double right_sample =
            lin_terp(0, 1, sosc->afd.filecontents[read_idx_right],
                     sosc->afd.filecontents[next_read_idx_right], frac);

        return_val = (left_sample + right_sample) / 2;
        sosc->m_read_idx += sosc->osc.m_inc;
    }

    return return_val;
}

void sampleosc_update(oscillator *self)
{
    sample_oscillator *sosc = (sample_oscillator *)self;
    osc_update(&sosc->osc);

    if (sosc->is_pitchless)
    {
        sosc->osc.m_inc = 1;
        return;
    }

    double unity_freq =
        sosc->is_single_cycle
            ? (SAMPLE_RATE / (sosc->afd.samplecount / sosc->afd.channels))
            : get_midi_freq(60); // middle c

    double length = SAMPLE_RATE / unity_freq;

    sosc->osc.m_inc *= length;

    if (sosc->m_read_idx >= sosc->afd.samplecount)
    {
        sosc->m_read_idx -= sosc->afd.samplecount;
    }
}

double sampleosc_do_oscillate(oscillator *self, double *quad_phase_output)
{

    if (quad_phase_output)
        *quad_phase_output = 0.0;

    if (!self->m_note_on)
        return 0.0;

    sampleosc *sosc = (sampleosc *)self;

    if (sosc->m_read_idx < 0)
        return 0.0;

    double left_output = sampleosc_read_sample_buffer(sosc);
    // double right_output;

    // check for wrap
    if (sosc->loop_mode == SAMPLE_ONESHOT)
    {
        if (sosc->m_read_idx >
            (double)(sosc->afd.samplecount - sosc->afd.channels - 1) /
                sosc->afd.channels)
            sosc->m_read_idx = -1;
    }
    else
    {
        if (sosc->m_read_idx >
            (double)(sosc->afd.samplecount - sosc->afd.channels - 1) /
                sosc->afd.channels)
            sosc->m_read_idx = 0;
    }

    return left_output;
}
