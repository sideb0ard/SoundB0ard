#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "spork.h"
#include "utils.h"

spork *new_spork()
{
    spork *s = (spork *)calloc(1, sizeof(spork));

    osc_new_settings((oscillator *)&s->m_lfo);
    lfo_set_soundgenerator_interface(&s->m_lfo);
    
    osc_new_settings(&s->m_osc1.osc);
    qb_set_soundgenerator_interface(&s->m_osc1);

    filter_moog_init(&s->m_filter);

    s->m_osc1.osc.m_waveform = SAW1;

    lfo_start_oscillator((oscillator*)&s->m_lfo);
    qb_start_oscillator((oscillator*)&s->m_osc1);

    s->m_lfo.osc.m_note_on = true;
    s->m_osc1.osc.m_note_on = true;

    s->sg.gennext = &spork_gennext;
    s->sg.status = &spork_status;
    s->sg.getvol = &spork_getvol;
    s->sg.setvol = &spork_setvol;
    s->sg.type = LFO_TYPE;

    s->m_volume = 0.7;

    return s;
}

double spork_gennext(void *sg)
{
    spork *s = (spork*) sg;
    double out = 0.0;
    double unused_quad = 0.0;

    osc_update(&s->m_lfo.osc, "SpoooooooOOOrkFO");
    out = lfo_do_oscillate(&s->m_lfo.osc, &unused_quad);

    return out * s->m_volume;

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

