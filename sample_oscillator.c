#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sample_oscillator.h"
#include "utils.h"

void sampleosc_init(sampleosc *sosc, char *filename)
{
    sampleosc_set_oscillator_interface(sosc);
    audiofile_data_import_file_contents(&sosc->afd, filename);
    sosc->m_read_idx = 0;
}

void sampleosc_set_oscillator_interface(sampleosc *sosc)
{
    sosc->osc.do_oscillate = &sampleosc_do_oscillate;
    sosc->osc.start_oscillator = &sampleosc_start_oscillator;
    sosc->osc.stop_oscillator = &sampleosc_stop_oscillator;
    sosc->osc.reset_oscillator = &sampleosc_reset_oscillator;
    sosc->osc.update_oscillator = &sampleosc_update;
}

double sampleosc_do_oscillate(oscillator *self, double *quad_phase_output)
{
    sampleosc_update(self);

    double left_output;
    // double right_output;

    if (!self->m_note_on)
        return 0.0;
    if (quad_phase_output)
        *quad_phase_output = 0.0;

    sampleosc *sosc = (sampleosc *)self;
    left_output = sampleosc_read_sample_buffer(sosc);

    return left_output;
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
    sosc->m_read_idx = 0;
}

double sampleosc_read_sample_buffer(sampleosc *sosc)
{
    double return_val = 0;

    unsigned int read_idx = sosc->m_read_idx;
    double frac = sosc->m_read_idx - read_idx;

    if (sosc->afd.channels == 1)
    {
        int next_read_idx =
            read_idx + 1 > sosc->afd.samplecount - 1 ? 0 : read_idx + 1;
        return_val = lin_terp(0, 1, sosc->afd.filecontents[read_idx],
                              sosc->afd.filecontents[next_read_idx], frac);
    }

    return return_val;
}

void sampleosc_update(oscillator *self)
{
    sample_oscillator *sosc = (sample_oscillator *)self;
    osc_update(&sosc->osc);

    sosc->m_read_idx += 1.0 * sosc->afd.channels;
    if (sosc->m_read_idx >= sosc->afd.samplecount)
    {
        sosc->m_read_idx -= sosc->afd.samplecount;
    }
}
