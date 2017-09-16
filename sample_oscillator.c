#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sample_oscillator.h"
#include "utils.h"

sampleosc *sampleosc_new()
{
    sampleosc *sosc = (sampleosc *)calloc(1, sizeof(sampleosc));
    if (sosc == NULL)
    {
        printf("Dinghie, mate\n");
        return NULL;
    }

    sampleosc_set_soundgenerator_interface(sosc);

    return sosc;
}

void sampleosc_set_soundgenerator_interface(sampleosc *sosc)
{
    sosc->osc.do_oscillate = &sampleosc_do_oscillate;
    sosc->osc.start_oscillator = &sampleosc_start_oscillator;
    sosc->osc.stop_oscillator = &sampleosc_stop_oscillator;
    sosc->osc.reset_oscillator = &sampleosc_reset_oscillator;
    sosc->osc.update_oscillator = &osc_update; // from base class
}

double sampleosc_do_oscillate(oscillator *self, double *quad_phase_output);
void sampleosc_start_oscillator(oscillator *self);
void sampleosc_stop_oscillator(oscillator *self);
void sampleosc_reset_oscillator(oscillator *self);
