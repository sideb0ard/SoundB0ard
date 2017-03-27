#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "spork.h"
#include "utils.h"

spork *new_spork()
{
    spork *s = (spork *)calloc(1, sizeof(spork));

    // LFO
    osc_new_settings((oscillator *)&s->m_lfo);
    lfo_set_soundgenerator_interface(&s->m_lfo);
    
    // THREE OSC
    osc_new_settings(&s->m_osc1.osc);
    qb_set_soundgenerator_interface(&s->m_osc1);
    s->m_osc1.osc.m_waveform = SINE;

    osc_new_settings(&s->m_osc2.osc);
    qb_set_soundgenerator_interface(&s->m_osc2);
    s->m_osc2.osc.m_waveform = SINE;
    s->m_osc2.osc.m_cents = 0.2;

    osc_new_settings(&s->m_osc3.osc);
    qb_set_soundgenerator_interface(&s->m_osc3);
    s->m_osc3.osc.m_waveform = SQUARE;

    filter_moog_init(&s->m_filter);


    lfo_start_oscillator((oscillator*)&s->m_lfo);
    qb_start_oscillator((oscillator*)&s->m_osc1);
    qb_start_oscillator((oscillator*)&s->m_osc2);
    qb_start_oscillator((oscillator*)&s->m_osc3);

    s->sg.gennext = &spork_gennext;
    s->sg.status = &spork_status;
    s->sg.getvol = &spork_getvol;
    s->sg.setvol = &spork_setvol;
    s->sg.type = SPORK_TYPE;

    s->m_volume = 0.0;

    return s;
}

double spork_gennext(void *sg)
{
    spork *s = (spork*) sg;

    // LFOOOOOO
    osc_update(&s->m_lfo.osc, "SpoooooooOOOrkFO");
    double unused_quad = 0.0;
    double lfo_out = lfo_do_oscillate(&s->m_lfo.osc, &unused_quad);

    //// update OSC
    s->m_osc1.osc.m_fo_mod = lfo_out;
    osc_update((oscillator *)&s->m_osc1, "OSC1");
    s->m_osc2.osc.m_fo_mod = lfo_out;
    osc_update((oscillator *)&s->m_osc2, "OSC2");
    s->m_osc3.osc.m_fo_mod = lfo_out;
    osc_update((oscillator *)&s->m_osc3, "OSC3");
    double osc_out =  0.3 * qb_do_oscillate((oscillator *)&s->m_osc1, NULL)
                    + 0.3 * qb_do_oscillate((oscillator *)&s->m_osc2, NULL)
                    + 0.3 * qb_do_oscillate((oscillator *)&s->m_osc3, NULL);

    // FILTERRZZZ
    moog_update((filter *)&s->m_filter);
    double filter_out = moog_gennext((filter *)&s->m_filter, osc_out);

    return filter_out * s->m_volume;

}

void spork_status(void *self, wchar_t *ss)
{
    spork *s = (spork*) self;
    swprintf(ss, MAX_PS_STRING_SZ, WANSI_COLOR_GREEN
             "[SPORK] Vol: %.2lf\n", s->m_volume);
    wcscat(ss, WANSI_COLOR_RESET);
}


double spork_getvol(void *self)
{
    spork *s = (spork*) self;
    return s->m_volume;
}

void spork_setvol(void *self, double v)
{
    spork *s = (spork*) self;
    s->m_volume = v;
}

