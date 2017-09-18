#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sample_oscillator.h"
#include "utils.h"

void sampleosc_init(sampleosc *sosc, char *filename)
{
    sampleosc_set_oscillator_interface(sosc);
    audiofile_data_import_file_contents(&sosc->afd, filename);
}

void sampleosc_set_oscillator_interface(sampleosc *sosc)
{
    sosc->osc.do_oscillate = &sampleosc_do_oscillate;
    sosc->osc.start_oscillator = &sampleosc_start_oscillator;
    sosc->osc.stop_oscillator = &sampleosc_stop_oscillator;
    sosc->osc.reset_oscillator = &sampleosc_reset_oscillator;
    sosc->osc.update_oscillator = &osc_update; // from base class
}

double sampleosc_do_oscillate(oscillator *self, double *quad_phase_output)
{
    return 0.4;
}

void sampleosc_start_oscillator(oscillator *self) {}
void sampleosc_stop_oscillator(oscillator *self) {}

void sampleosc_reset_oscillator(oscillator *self) {}
